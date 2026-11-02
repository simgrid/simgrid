#!/usr/bin/env python3
#
# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.
"""Generate SMPI collective selectors from an MVAPICH tuning JSON tree.

MVAPICH (>= 4.x) does not carry C tuning tables anymore: collective algorithms are
chosen by a declarative decision tree stored in JSON and evaluated on every call by
the csel machinery (src/mpi/coll/src/csel.c). This script walks such a tree and emits
the equivalent if/else selector for one collective, so that SMPI can follow a given
MVAPICH tuning without transcoding thresholds by hand.

    tools/smpi/gen_mvapich2_selector.py \\
        --json maint/tuning/coll/mpir/mvp_generic.json \\
        --algorithms src/mpi/coll/coll_algorithms.txt \\
        -o src/smpi/colls/smpi_mvapich2_selector.cpp

All the collective selectors land in that one file, the way every other vendor
selector (smpi_mpich_selector.cpp, smpi_openmpi_selector.cpp, ...) is laid out. Pass
--collective to regenerate a single one.

Every algorithm the tree selects must be mapped explicitly to an SMPI kernel: an
unknown name is a hard error, so the generated selector can never silently fall back
to a kernel the tuning did not choose. Algorithms whose `restrictions' (commutative,
builtin-op, parent-comm, ...) contradict the branch they appear in are dropped, as
MPIR_Csel_search() does. Branches that end up selecting the same kernel as their
fallback are pruned, so the emitted code stays readable.
"""

import argparse
import json
import os
import re
import sys

# Collective -> {MVAPICH algorithm: (SMPI kernel, parameters the kernel honours)}
MAPPINGS = {
    'allreduce': {
        'MPIR_Allreduce_intra_recursive_doubling': ('allreduce__rdb', []),
        'MPIR_Allreduce_intra_reduce_scatter_allgather': ('allreduce__rab_rdb', []),
        'MPIR_Allreduce_intra_ring': ('allreduce__ring', []),
        'MPIR_Allreduce_intra_smp': ('allreduce__smp_rsag', []),
        'MPIR_Allreduce_intra_recursive_multiplying': ('allreduce__recursive_multiplying', ['k']),
        'MPIR_Allreduce_intra_recexch': ('allreduce__recexch(sendbuf, recvbuf, count, datatype, op, comm, %(k|2)s)',
                                         ['k']),
        # MPICH's k-RSAG is reduce-scatter-allgather at k=2; the k value is not a parameter
        # of the SMPI kernel, so the generated code records it as ignored.
        'MPIR_Allreduce_intra_k_reduce_scatter_allgather': ('allreduce__rab_rdb', []),
        # SMPI models blocking collectives only; MVAPICH's nonblocking algorithm is a
        # scheduling choice with the same semantics, so it maps onto recursive doubling.
        'MPIR_Allreduce_allcomm_nb': ('allreduce__rdb', []),
        # SMPI's tree reduction is a binomial reduce + binomial broadcast: the kary/chunk
        # parameters of MVAPICH's pipelined tree are not modelled.
        'MPIR_Allreduce_intra_tree': ('allreduce__tree', []),
    },
    'bcast': {
        'MPIR_Bcast_intra_binomial': ('bcast__binomial_tree', []),
        # kary tree; MPICH's whole-message variant, k is the branching factor
        'MPIR_Bcast_intra_tree': ('bcast__tree(buf, count, datatype, root, comm, %(k|2)s, 0)', ['k']),
        'MPIR_Bcast_intra_pipelined_tree': (
            'bcast__tree(buf, count, datatype, root, comm, %(k|2)s, %(chunk_size|8192)s)', ['k', 'chunk_size']),
        'MPIR_Bcast_intra_scatter_recursive_doubling_allgather': ('bcast__scatter_rdb_allgather', []),
        'MPIR_Bcast_intra_scatter_ring_allgather': ('bcast__scatter_LR_allgather', []),
        'MPIR_Bcast_intra_smp': ('bcast__mvapich_smp', []),
        'MPIR_Bcast_intra_osu_knomial': (
            'bcast__knomial(buf, count, datatype, root, comm, %(intra_kf|4)s, 0)', ['intra_kf', 'inter_kf']),
        'MPIR_Bcast_intra_osu_pipelined': (
            'bcast__knomial(buf, count, datatype, root, comm, %(intra_kf|4)s, %(seg_size|8192)s)',
            ['intra_kf', 'seg_size']),
    },
    'allgather': {
        'MPIR_Allgather_intra_brucks': ('allgather__bruck', []),
        'MPIR_Allgather_intra_recursive_doubling': ('allgather__rdb', []),
        'MPIR_Allgather_intra_ring': ('allgather__ring', []),
        # MVAPICH's osu_direct sends each block straight to every peer; SMPI has no such
        # kernel, NTSLR_NB is the closest (a ring shift of the whole buffer). MVAPICH only
        # selects it on single-rank communicators, where any kernel is equivalent.
        'MPIR_Allgather_intra_osu_direct': ('allgather__NTSLR_NB', []),
    },
    'allgatherv': {
        'MPIR_Allgatherv_intra_brucks': ('allgatherv__ompi_bruck', []),
        'MPIR_Allgatherv_intra_recursive_doubling': ('allgatherv__mpich_rdb', []),
        'MPIR_Allgatherv_intra_ring': ('allgatherv__mpich_ring', []),
    },
    'gather': {
        'MPIR_Gather_intra_binomial': ('gather__ompi_binomial', []),
        # osu_direct is root-receives-from-everyone, i.e. exactly the linear kernel
        'MPIR_Gather_intra_osu_direct': ('gather__ompi_basic_linear', []),
    },
    'scatter': {
        'MPIR_Scatter_intra_binomial': ('scatter__ompi_binomial', []),
        'MPIR_Scatter_intra_osu_direct': ('scatter__ompi_basic_linear', []),
    },
    'reduce': {
        'MPIR_Reduce_intra_binomial': ('reduce__binomial', []),
        'MPIR_Reduce_intra_reduce_scatter_gather': ('reduce__scatter_gather', []),
        # SMPI's smp reduce is MVAPICH's node-aware one, not MPICH's
        'MPIR_Reduce_intra_smp': ('reduce__mpich_smp', []),
    },
    'reduce_scatter': {
        'MPIR_Reduce_scatter_intra_recursive_halving': ('reduce_scatter__ompi_basic_recursivehalving', []),
        'MPIR_Reduce_scatter_intra_pairwise': ('reduce_scatter__mpich_pair', []),
        'MPIR_Reduce_scatter_intra_noncommutative': ('reduce_scatter__mpich_noncomm', []),
        'MPIR_Reduce_scatter_intra_recursive_doubling': ('reduce_scatter__mpich_rdb', []),
    },
    'alltoall': {
        'MPIR_Alltoall_intra_pairwise': ('alltoall__pairwise', []),
        'MPIR_Alltoall_intra_scattered': ('alltoall__scattered', []),
        'MPIR_Alltoall_intra_k_brucks': (
            'alltoall__k_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, %(k|2)s)', ['k']),
        # sendrecv_replace only differs in how it saves the local block; the schedule is pairwise
        'MPIR_Alltoall_intra_pairwise_sendrecv_replace': ('alltoall__pairwise', []),
    },
    'alltoallv': {
        'MPIR_Alltoallv_intra_scattered': ('alltoallv__scattered', []),
        # SMpi has no separate sendrecv_replace kernel for alltoallv; the ring does the
        # same local copy followed by rank+-step exchanges.
        'MPIR_Alltoallv_intra_pairwise_sendrecv_replace': ('alltoallv__ring', []),
    },
    'barrier': {
        'MPIR_Barrier_intra_k_dissemination': ('barrier__k_dissemination(comm, %(k|2)s)', ['k']),
        # SMPI's smp barrier is MVAPICH's two-level one, not MPICH's
        'MPIR_Barrier_intra_smp': ('barrier__mpich_smp', []),
    },
}

# Collective -> (parameter list, locals derived from the call)
SIGNATURES = {
    'allreduce': {
        'sig': 'const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm',
        'args': 'sendbuf, recvbuf, count, datatype, op, comm',
        'locals': ['int comm_size    = comm->size();',
                   'long nbytes      = static_cast<long>(count) * datatype->size();',
                   'bool commutative = (op == MPI_OP_NULL || op->is_commutative());',
                   'bool builtin_op  = (op != MPI_OP_NULL && op->is_predefined());']},
    'bcast': {
        'sig': 'void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm',
        'args': 'buf, count, datatype, root, comm',
        'locals': ['int comm_size = comm->size();',
                   'long nbytes   = static_cast<long>(count) * datatype->size();']},
    'allgather': {
        'sig': 'const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, '
               'MPI_Datatype recvtype, MPI_Comm comm',
        'args': 'sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm',
        'locals': ['int comm_size = comm->size();',
                   'long nbytes   = static_cast<long>(recvcount) * recvtype->size();']},
    'allgatherv': {
        'sig': 'const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, '
               'const int *displs, MPI_Datatype recvtype, MPI_Comm comm',
        'args': 'sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm',
        'locals': ['int comm_size     = comm->size();',
                   'long total_count  = 0; for (int i = 0; i < comm_size; i++) total_count += recvcounts[i];',
                   'long nbytes       = static_cast<long>(sendcount) * sendtype->size();'],
        # csel sums the receive counts for allgatherv instead of multiplying by the size
        'total': 'total_count * recvtype->size()'},
    'gather': {
        'sig': 'const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, '
               'MPI_Datatype recvtype, int root, MPI_Comm comm',
        'args': 'sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm',
        'locals': ['int comm_size = comm->size();',
                   'long nbytes   = static_cast<long>(sendcount) * sendtype->size();']},
    'scatter': {
        'sig': 'const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, '
               'MPI_Datatype recvtype, int root, MPI_Comm comm',
        'args': 'sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm',
        'locals': ['int comm_size = comm->size();',
                   'long nbytes   = static_cast<long>(sendcount) * sendtype->size();']},
    'reduce': {
        'sig': 'const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, '
               'MPI_Comm comm',
        'args': 'sendbuf, recvbuf, count, datatype, op, root, comm',
        'locals': ['int comm_size    = comm->size();',
                   'long nbytes      = static_cast<long>(count) * datatype->size();',
                   'bool commutative = (op == MPI_OP_NULL || op->is_commutative());',
                   'bool builtin_op  = (op != MPI_OP_NULL && op->is_predefined());']},
    'reduce_scatter': {
        'sig': 'const void *sendbuf, void *recvbuf, const int *rcounts, MPI_Datatype datatype, MPI_Op op, '
               'MPI_Comm comm',
        'args': 'sendbuf, recvbuf, rcounts, datatype, op, comm',
        'locals': ['int comm_size        = comm->size();',
                   'long total_count     = 0; for (int i = 0; i < comm_size; i++) total_count += rcounts[i];',
                   'bool commutative     = (op == MPI_OP_NULL || op->is_commutative());',
                   'bool block_regular   = true; '
                   'for (int i = 1; i < comm_size; i++) if (rcounts[i] != rcounts[0]) block_regular = false;'],
        'total': 'total_count * datatype->size()'},
    'alltoall': {
        'sig': 'const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, '
               'MPI_Datatype recvtype, MPI_Comm comm',
        'args': 'sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm',
        'locals': ['int comm_size = comm->size();',
                   'long nbytes   = static_cast<long>(sendcount) * sendtype->size();']},
    'alltoallv': {
        'sig': 'const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, '
               'void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm',
        'args': 'sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm',
        'locals': ['int comm_size    = comm->size();',
                   'long total_count = 0; for (int i = 0; i < comm_size; i++) total_count += sendcounts[i];',
                   'long nbytes      = static_cast<long>(sendcounts[comm->rank()]) * sendtype->size();'],
        'total': 'total_count * sendtype->size()'},
    'barrier': {
        'sig': 'MPI_Comm comm',
        'args': 'comm',
        'locals': ['int comm_size = comm->size();']},
}

# Where the JSON picks nothing SMPI can execute (an algorithm pruned by its restrictions,
# or an inter-communicator branch), each selector falls back to a safe kernel of its own.
FALLBACK = {'allreduce': 'rdb', 'bcast': 'binomial_tree', 'allgather': 'bruck', 'allgatherv': 'ompi_bruck',
            'gather': 'ompi_binomial', 'scatter': 'ompi_binomial', 'reduce': 'binomial',
            'reduce_scatter': 'ompi_basic_recursivehalving', 'alltoall': 'scattered',
            'alltoallv': 'scattered', 'barrier': 'mpich_smp'}

# Predicate -> C++ variable, and the locals needed to compute it.
PRED_VAR = {'comm_size': 'comm_size', 'avg_msg_size': 'nbytes', 'total_msg_size': 'total_nbytes',
            'count': 'count', 'comm_avg_ppn': 'avg_ppn'}
PRED_LOCAL = {
    'avg_ppn': 'int avg_ppn = comm->get_leaders_comm() != MPI_COMM_NULL && '
               'comm->get_leaders_comm()->size() > 0 ? comm_size / comm->get_leaders_comm()->size() : comm_size;',
    'total_nbytes': 'long total_nbytes = nbytes * comm_size;',
}
BOOL_PRED = {
    'is_commutative': ('commutative', '!commutative'),
    'is_op_built_in': ('builtin_op', '!builtin_op'),
    'is_sendbuf_inplace': ('sendbuf == MPI_IN_PLACE', 'sendbuf != MPI_IN_PLACE'),
    'is_block_regular': ('block_regular', '!block_regular'),
}


def die(msg):
    sys.stderr.write('gen_mvapich2_selector: %s\n' % msg)
    raise SystemExit(1)


def split_predicate(key):
    """'avg_msg_size<2048' -> ('avg_msg_size', '<', '2048'); 'comm_size=any' -> ('comm_size', None, 'any')"""
    if key.endswith('=any'):
        return key[:-len('=any')], None, 'any'
    if key.startswith('algorithm='):
        return None, None, None
    m = re.match(r'^([a-z_]+)=([a-z_0-9]+)$', key)
    if m:
        return m.group(1), None, m.group(2)
    m = re.match(r'^([a-z_]+?)(<=|<|>=|>|==)([0-9]+)$', key)
    if m:
        return m.group(1), m.group(2), m.group(3)
    m = re.match(r'^([a-z_]+?)(<=|<|>=|>|==)([a-z_0-9]+)$', key)   # symbolic bound, e.g. count<pow2
    if m:
        return m.group(1), m.group(2), m.group(3)
    return None, None, None


def load_restrictions(path):
    """coll_algorithms.txt -> {'reduce': {'intra': {'smp': {'commutative'}, ...}}}"""
    restrictions = {}
    coll = kind = algo = None
    for line in open(path):
        m = re.match(r'^([a-z_0-9]+)-(intra|inter):\s*$', line)
        if m:
            coll, kind, algo = m.group(1), m.group(2), None
            restrictions.setdefault(coll, {}).setdefault(kind, {})
            continue
        m = re.match(r'^    ([a-z_0-9]+)\s*$', line)
        if m and coll and kind:
            algo = m.group(1)
            restrictions[coll][kind].setdefault(algo, set())
            continue
        m = re.match(r'^        restrictions:\s*(.+)$', line)
        if m and coll and kind and algo:
            restrictions[coll][kind][algo] = {r.strip() for r in m.group(1).split(',')}
    return restrictions


def alg_key(name):
    """MPIR_Allreduce_intra_smp -> ('allreduce', 'smp')"""
    m = re.match(r'^MPIR_([A-Za-z]+)_(?:intra|inter|allcomm)_(.+)$', name)
    if not m:
        return None, None
    return re.sub(r'(?<!^)(?=[A-Z])', '_', m.group(1)).lower(), m.group(2)


def violated(restrictions, coll, algo, assumptions):
    needed = restrictions.get(coll, {}).get('intra', {}).get(algo, set())
    bad = set()
    if 'commutative' in needed and assumptions.get('is_commutative') == 'no':
        bad.add('commutative')
    if 'builtin-op' in needed and assumptions.get('is_op_built_in') == 'no':
        bad.add('builtin-op')
    if 'inplace' in needed and assumptions.get('is_sendbuf_inplace') == 'no':
        bad.add('inplace')
    if 'no-inplace' in needed and assumptions.get('is_sendbuf_inplace') == 'yes':
        bad.add('no-inplace')
    if 'parent-comm' in needed and assumptions.get('comm_hierarchy') == 'child':
        bad.add('parent-comm')
    return bad


class Generator:
    """Turns the JSON subtree into a small AST, simplifies it, then renders C++."""

    def __init__(self, coll, name, mapping, restrictions, fallback, trace=False, hierarchy='any'):
        self.coll = coll
        self.base_locals = SIGNATURES[coll]['locals']
        self.total_expr = SIGNATURES[coll].get('total', 'nbytes * comm_size')
        self.name = name
        self.trace = trace
        self.hierarchy = hierarchy
        self.mapping = mapping
        self.restrictions = restrictions
        self.fallback = fallback
        self.locals = []
        self.stats = {'leaves': 0, 'mapped': 0, 'pruned': 0, 'inter': 0, 'collapsed': 0}
        self.kernels = {}

    # ---- JSON -> AST -----------------------------------------------------------------
    def build(self, node, assumptions):
        keys = [k for k in node if not k.startswith('algorithm=')]
        leaf = next((k for k in node if k.startswith('algorithm=')), None)
        if leaf is not None and not keys:
            return self.leaf(node, leaf, assumptions)

        # csel evaluates the keys in the order they appear, so the generic case must too.
        # Only the boolean pairs (is_commutative=yes/no, comm_hierarchy=parent/child) are
        # gathered, since they collapse into one if/else.
        entries, default, handled = [], None, set()
        for key in keys:
            name, op, val = split_predicate(key)
            if name is None:
                die('unhandled predicate %r in %s' % (key, self.coll))
            if name == 'comm_type':
                if val == 'intra':
                    return self.build(node[key], assumptions)
                continue  # inter-communicator subtrees are not modelled by SMPI selectors
            if name in BOOL_PRED or name == 'comm_hierarchy':
                if name in handled:
                    continue
                handled.add(name)
                if name == 'comm_hierarchy' and self.hierarchy != 'both':
                    # user-level communicators are never 'parent' comms in SMPI: the
                    # intra-node selector is generated separately, with --hierarchy parent
                    wanted = 'parent' if self.hierarchy == 'parent' else 'any'
                    key = next((k for k in keys if split_predicate(k)[0] == name
                                and split_predicate(k)[2] == wanted), None)
                    if key is not None:
                        default = self.build(node[key], assumptions)
                    continue
                yes = next((k for k in keys if split_predicate(k)[0] == name
                            and split_predicate(k)[2] in ('yes', 'parent')), None)
                no = next((k for k in keys if split_predicate(k)[0] == name
                           and split_predicate(k)[2] in ('no', 'child', 'any')), None)
                if yes is not None:
                    inner = dict(assumptions)
                    inner[name] = split_predicate(yes)[2]
                    entries.append((self.condition(name, None, split_predicate(yes)[2]),
                                    self.build(node[yes], inner)))
                if no is not None:
                    inner = dict(assumptions)
                    inner[name] = split_predicate(no)[2]
                    default = self.build(node[no], inner)
                continue
            if val == 'any':
                if default is None:
                    default = self.build(node[key], assumptions)
                break  # everything after a '=any' key is unreachable
            inner = dict(assumptions)
            inner[name] = val
            entries.append((self.condition(name, op, val), self.build(node[key], inner)))
        if not entries:
            return default if default is not None else ('leaf', self.fallback, '', ())
        return ('branch', entries, default)

    def condition(self, name, op, val):
        if val == 'pow2':                      # csel: comm_size is a power of two / count below pof2
            if name == 'comm_size':
                return '(comm_size & (comm_size - 1)) == 0'
            if name == 'count':
                local = 'int pof2 = 1; while (pof2 * 2 <= comm_size) pof2 *= 2;'
                if local not in self.locals:
                    self.locals.append(local)
                return 'count < pof2'
            die('unhandled pow2 predicate on %r' % name)
        if name in BOOL_PRED:
            return BOOL_PRED[name][0] if val in ('yes', '1') else BOOL_PRED[name][1]
        if name == 'comm_hierarchy':
            return 'comm->is_smp_comm()' if val == 'parent' else '!comm->is_smp_comm()'
        var = PRED_VAR.get(name)
        if var is None:
            die('unhandled predicate %r' % name)
        if var == 'total_nbytes':
            local = 'long total_nbytes = %s;' % self.total_expr
        else:
            local = PRED_LOCAL.get(var)
        if local and local not in self.locals:
            self.locals.append(local)
        return '%s %s %s' % (var, op or '<', val)

    def leaf(self, node, leaf, assumptions):
        self.stats['leaves'] += 1
        name = leaf[len('algorithm='):]
        _, algo = alg_key(name)
        if '_inter_' in name:
            self.stats['inter'] += 1
            return ('leaf', self.fallback, '/* %s: inter-communicator branch */' % algo)
        bad = violated(self.restrictions, self.coll, algo, assumptions)
        if bad:
            self.stats['pruned'] += 1
            return ('leaf', self.fallback, '/* %s needs %s */' % (algo, ', '.join(sorted(bad))))
        entry = self.mapping.get(name)
        if entry is None:
            die('%s has no SMPI kernel; add it to the mapping of %s' % (name, self.coll))
        template, honoured = entry
        params = [k for k in node[leaf] if not k.startswith('algorithm=')]
        values = dict(p.split('=', 1) for p in params if '=' in p)
        ignored = [p for p in params if p.split('=')[0] not in honoured]
        kernel = re.sub(r'%\(([a-z_0-9]+)(?:\|([^)]*))?\)s',
                        lambda m: values.get(m.group(1), m.group(2) or '0'), template)
        name_only = kernel.split('(')[0]
        self.stats['mapped'] += 1
        self.kernels[name_only] = self.kernels.get(name_only, 0) + 1
        return ('leaf', kernel, '/* %s ignored by the kernel */' % ', '.join(ignored) if ignored else '')

    # ---- AST simplification ----------------------------------------------------------
    def simplify(self, ast):
        if ast[0] == 'leaf':
            return ast
        entries = [(c, self.simplify(ch)) for c, ch in ast[1]]
        default = self.simplify(ast[2]) if ast[2] is not None else None

        # A branch that selects what falling through would select anyway is redundant.
        rest = default
        kept = []
        for cond, child in reversed(entries):
            if rest is not None and child[1:] == rest[1:]:
                self.stats['collapsed'] += 1
                continue
            kept.append((cond, child))
            rest = child
        entries = list(reversed(kept))

        if not entries:
            return default if default is not None else ('leaf', self.fallback, '')
        if default is not None and all(ch[1:] == default[1:] for _, ch in entries):
            return default
        return ('branch', entries, default)

    # ---- AST -> C++ ------------------------------------------------------------------
    def render(self, ast, indent, out):
        pad = '  ' * indent
        if ast[0] == 'leaf':
            call = ast[1] if '(' in ast[1] else '%s(%s)' % (ast[1], SIGNATURES[self.coll]['args'])
            if self.trace:
                out.append('%sXBT_VERB("mvapich2: %s");' % (pad, ast[1].split('(')[0]))
            note = (' ' + ast[2]) if ast[2] else ''
            out.append('%sreturn %s;%s' % (pad, call, note))
            return
        entries, default = ast[1], ast[2]
        for idx, (cond, child) in enumerate(entries):
            out.append('%s%s (%s) {' % (pad, 'if' if idx == 0 else '} else if', cond))
            self.render(child, indent + 1, out)
        out.append('%s}' % pad)
        self.render(default if default is not None else ('leaf', self.fallback, ''), indent, out)

    def emit(self, tree):
        """Render one selector as a function: returns the declaration line and the body."""
        signature = SIGNATURES[self.coll]
        ast = self.simplify(self.build(tree, {}))
        body = []
        self.render(ast, 1, body)
        text = '\n'.join(body)
        # only keep the locals the simplified tree actually uses; one local can be needed
        # only by another one (total_nbytes needs total_count), so grow the searched text
        # until no new local matches
        candidates = self.base_locals + self.locals
        used, search = [], text
        while True:
            added = [line for line in candidates
                     if line not in used and re.search(r'\b%s\b' % declared_name(line), search)]
            if not added:
                break
            used.extend(added)
            search += '\n' + '\n'.join(added)
        used.sort(key=candidates.index)
        lines = ['  ' + line for line in used] + body
        return 'int %s(%s)' % (self.name, signature['sig']), lines


def declared_name(line):
    match = re.match(r'.*?\b(\w+)\s*=', line)
    return match.group(1) if match else ''


HEADER = ['/* selector for collective algorithms based on mvapich decision logic */', '',
          '/* Copyright (c) 2009-2026. The SimGrid Team. All rights reserved.          */', '',
          '/* This program is free software; you can redistribute it and/or modify it',
          ' * under the terms of the license (GNU LGPL) which comes with this package. */', '',
          '/* Generated by tools/smpi/gen_mvapich2_selector.py from @JSON@, which holds the',
          ' * tuning MVAPICH enables by default on a plain build.',
          ' * Do not edit by hand: rerun the generator instead. */', '',
          '#include "colls_private.hpp"', '',
          'namespace simgrid::smpi {', '']

FOOTER = ['} // namespace simgrid::smpi', '']


def split_functions(text, wanted):
    """Existing generated file -> {collective: (declaration, body lines)} for the wanted ones."""
    found = {}
    lines = text.split('\n')
    for idx, line in enumerate(lines):
        m = re.match(r'^(int ([a-z_]+)__mvapich2\(.*)$', line)
        if not m or m.group(2) not in wanted:
            continue
        body, depth = [], 0
        for inner in lines[idx + 1:]:
            if depth == 0 and inner == '}':
                break
            depth += inner.count('{') - inner.count('}')
            body.append(inner)
        found[m.group(2)] = (m.group(1), body)
    return found.items()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--json', required=True, help='MVAPICH tuning JSON (mpir/mvp_generic.json, tacc_vista.json, ...)')
    ap.add_argument('--algorithms', required=True, help='MVAPICH coll_algorithms.txt, for the restrictions')
    ap.add_argument('--collective', action='append', choices=sorted(MAPPINGS), metavar='COLL',
                    help='regenerate only these collectives (default: all of them)')
    ap.add_argument('-o', '--output', required=True, help='SMPI selector file to (re)write')
    ap.add_argument('--fallback', metavar='KERNEL',
                    help='SMPI kernel used where the tree is not followed (default: a safe one per collective)')
    ap.add_argument('--trace', action='store_true', help='log the selected kernel (XBT_VERB), for validation')
    ap.add_argument('--hierarchy', choices=['any', 'parent', 'both'], default='any',
                    help="which comm_hierarchy branch to generate: 'any' (user-level comms, the default), "
                         "'parent' (intra-node comms, for a two-level kernel) or 'both'")
    args = ap.parse_args()

    tree = json.load(open(args.json))
    restrictions = load_restrictions(args.algorithms)
    collectives = args.collective or sorted(MAPPINGS)

    # Rewriting a single collective of an existing file leaves the others alone, so a
    # partial regeneration does not need the tuning for the collectives it is not editing.
    sections = {}
    if os.path.exists(args.output):
        for coll, section in split_functions(open(args.output).read(), set(collectives)):
            sections[coll] = section
    for coll in collectives:
        key = 'collective=%s' % coll
        if key not in tree:
            if coll in sections:
                sys.stderr.write('%s: not in %s, keeping the existing function\n' % (coll, args.json))
                continue
            die('%s not present in %s' % (key, args.json))

        gen = Generator(coll, '%s__mvapich2' % coll, MAPPINGS[coll], restrictions,
                        '%s__%s' % (coll, args.fallback or FALLBACK[coll]), trace=args.trace,
                        hierarchy=args.hierarchy)
        sections[coll] = gen.emit(tree[key])

        s = gen.stats
        sys.stderr.write('%-14s %3d leaves (%d mapped, %d pruned by restrictions, %d inter), '
                         '%d collapsed branches\n'
                         % (coll, s['leaves'], s['mapped'], s['pruned'], s['inter'], s['collapsed']))
        for kernel, hits in sorted(gen.kernels.items(), key=lambda kv: -kv[1]):
            sys.stderr.write('    %-32s %3d leaves\n' % (kernel, hits))

    out = [line.replace('@JSON@', os.path.basename(args.json)) for line in HEADER]
    for coll in sorted(sections):
        decl, body = sections[coll]
        out += [decl, '{'] + body + ['}', '']
    out += FOOTER
    open(args.output, 'w').write('\n'.join(out))


if __name__ == '__main__':
    main()
