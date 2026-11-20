# MPI correctness checks that SMPI does not do yet

Baseline: SimGrid master of 2026-11-20 (commit 59e54ea). The checks that SMPI does are documented in `docs/source/app_smpi.rst`
(section "Debugging MPI applications with SMPI"), and listed exhaustively by `grep -rn CHECK src/smpi`.

The identifiers are kept stable across versions of this list: E*n* (easy), M*n* (medium), H*n* (hard), NP*n* (not
planned). The source of each check is given in parentheses: ITAC tag, or MUST identifier.

## Easy

- **E10. Thread level not provided** (`MUST_WARNING_THREADLEVEL`, warning)
  - Why: `MPI_Init_thread` always provides `MPI_THREAD_SINGLE` in SMPI. An application asking for more and ignoring
    the `provided` argument may call MPI from several threads, which SMPI does not support.
  - How: warn in `PMPI_Init_thread` when `required` is above `MPI_THREAD_SINGLE`.

## Medium

- **M1. Space of in-flight buffered sends** (`LOCAL:BUFFER:INSUFFICIENT_BUFFER`, always)
  - Why: SMPI checks each buffered message alone, but the attached buffer must hold all messages not yet delivered.
  - How: account the bytes of each buffered send in the actor until its completion, and check their sum.

- **M4. Uninitialized data sent** (`LOCAL:MEMORY:INITIALIZATION`, under Valgrind)
  - Why: sending uninitialized memory silently propagates garbage to other ranks.
  - How: when running under Valgrind, `VALGRIND_CHECK_MEM_IS_DEFINED` on send buffers (contiguous types first).

- **M5. Buffer modified while owned by MPI** (`LOCAL:MEMORY:ILLEGAL_MODIFICATION`, pedantic)
  - Why: writing into a buffer between `MPI_Isend` and its completion is erroneous; SMPI's eager copy hides it.
  - How: checksum the send buffer at the start and at the completion of the operation (contiguous types first).

- **M6. Partial overlap of send and receive buffers in collectives** (`LOCAL:MEMORY:OVERLAP`, pedantic)
  - Why: overlapping buffers with different pointers are erroneous; SMPI only detects identical pointers.
  - How: byte intervals of both buffers (contiguous types), sorted and swept for an intersection. About 50 lines, for
    a rare bug.

- **M7. Overlapping concurrent nonblocking operations** (`LOCAL:MEMORY:OVERLAP`, pedantic)
  - Why: two pending operations on the same memory give undefined results.
  - How: byte ranges of each actor's active requests in an interval map; check at each start, remove at completion.

- **M8. Self-overlapping datatypes in receives** (MUST datatype checks, always)
  - Why: a datatype whose entries overlap (for example a vector whose stride is smaller than its block length) is
    erroneous in a receive, as a location would be written twice.
  - How: compute at commit time whether the type map of a derived datatype overlaps, store it as a flag, and check
    it in the receive and collective bindings.

- **M9. Struct datatype not matching the C struct** (`MUST_WARNING_DATATYPE_BAD_ALIGNMENT`, pedantic, warning)
  - Why: a datatype built with `MPI_Type_create_struct` whose extent ignores the padding of the C struct works for one
    element, but shifts every following element of an array.
  - How: at commit, compute the alignment of the members, and warn when the extent is not a multiple of it (the usual
    fix is `MPI_Type_create_resized` to `sizeof(struct)`).

## Hard

- **H1. Type signature matching for derived datatypes** (`GLOBAL:MSG:DATATYPE:MISMATCH`,
  `GLOBAL:COLLECTIVE:DATATYPE:MISMATCH`, pedantic)
  - Why: any derived datatype currently matches anything, so sending one struct as another is silent corruption.
  - How: compute and cache a signature per datatype (sequence of basic types, run-length encoded or hashed), and
    compare the sender's with a prefix of the receiver's, in P2P and in the per-pair collective check.

- **H2. Potential deadlocks, with a wait-for graph** (`GLOBAL:DEADLOCK:POTENTIAL`, MUST)
  - Why: finds deadlocks that did not happen in this run but may in another interleaving, with the AND/OR semantics of
    `MPI_Waitany` and wildcards. It would also detect cycles among some ranks while others still progress, which
    SimGrid only reports once all ranks are blocked.
  - How: MUST's wait-for graph over pending operations. SimGrid's model checker already explores interleavings, a
    heavier but more complete alternative.

- **H4. RMA (one-sided) races** (MUST, pedantic)
  - Why: conflicting `MPI_Put`, `MPI_Get` and local accesses within an epoch are undefined.
  - How: accessed ranges per window and epoch, checked at synchronization. A whole subsystem.

- **H5. Freed or read-only buffers** (`LOCAL:MEMORY:INACCESSIBLE`, under Valgrind or ASan)
  - Why: passing a buffer that cannot be read or written.
  - How: only feasible with Valgrind or ASan hooks, as M4.

- **H6. Application accessing memory owned by MPI** (`LOCAL:MEMORY:ILLEGAL_ACCESS`, under Valgrind)
  - Why: reading a receive buffer, or writing a send buffer, before the operation completes.
  - How: mark the buffers of pending operations as inaccessible for Valgrind (`VALGRIND_MAKE_MEM_NOACCESS`) and restore
    them at completion; covers reads, unlike M5.

- **H7. Buffer memory not matching the datatype** (MUST with TypeART, compile-time instrumentation)
  - Why: passing an array of `double` with `MPI_INT` corrupts data, and is invisible at the MPI level.
  - How: MUST relies on TypeART, an LLVM pass recording the type of each allocation. Possible with `smpicc`, but a
    project on its own.

## Not planned

- **NP1. Zero counts and zero-sized datatypes** (MUST, warning)
  - Why: sometimes a sign of an uninitialized variable or a wrong datatype.
  - Why not: legal and very common (empty blocks in v-collectives, boundary ranks), so the warnings would mostly be
    noise.

- **NP2. Too many live requests or datatypes** (`LOCAL:REQUEST:NOT_FREED`, `LOCAL:DATATYPE:NOT_FREED`)
  - Why: a number of live requests or datatypes growing without bound usually means a forgotten `MPI_Wait` or
    `MPI_Type_free`.
  - Why not: `smpi/list-leaks` already reports these leaks at the end of the simulation, with their allocation site
    when compiled with `-trace-call-location`. ITAC needs this warning because, on a real platform, the leak may exhaust
    the resources of the MPI library before the end.

- **NP3. Unusual but legal arguments** (several MUST warnings)
  - What: a logical argument that is neither 0 nor 1 (`INTEGER_NOT_ONE_OR_ZERO`), committing a predefined or already
    committed datatype (`DATATYPE_PREDEFINED`, `DATATYPE_COMMITED`), an empty group (`IF_EMPTY`), `MPI_COMM_NULL` where
    allowed (`COMM_NULL`), waiting on a null or inactive request (`REQUEST_NULL`, `REQUEST_INACTIVE`), canceling a
    request twice (`REQUEST_CANCELED`).
  - Why not: all legal, and rarely the cause of a bug; mostly noise.

- **NP4. Data races between MPI calls and threads** (MUST with TSan)
  - Why: a thread modifying a buffer while another thread's MPI call uses it.
  - How: ThreadSanitizer annotations around MPI buffer accesses; SMPI's actor model makes this case unusual.

- **NP5. Partitioned point-to-point checks** (MUST)
  - Why not: SMPI does not implement partitioned communication (`MPI_Psend_init`).

- **NP6. Data corrupted during transmission** (`GLOBAL:MSG:DATA_TRANSMISSION_CORRUPTED`,
  `GLOBAL:COLLECTIVE:DATA_TRANSMISSION_CORRUPTED`)
  - Why not: ITAC checksums the data on both sides to catch bugs of the network or of the MPI library. SMPI copies the
    data in memory; a modification by the application is M5.

- **NP7. No progress for a long time** (`GLOBAL:DEADLOCK:NO_PROGRESS`)
  - Why not: a simulation either deadlocks, which is detected, or progresses in simulated time.

## Related work that is not a check

- **Messages outside the macros**: some of the MUST checks are implemented as diagnostics written directly in the `.cpp` files
  of SMPI (requests, finalize, windows, datatypes). Some errors are even returned without any message, for example attaching a
  second buffer with `MPI_Buffer_attach` (`MUST_ERROR_BUFFER_REATTACH`), freeing a predefined operation
  (`MUST_ERROR_OPERATION_PREDEFINED`), or a direction out of range in `MPI_Cart_shift` (`MUST_ERROR_DIRECTION_GREATER_NDIMS`).

## ITAC coverage

ITAC 2021.10 defines 25 error tags, listed in its User and Reference Guide. ITAC is deprecated, so this list should
not change anymore.

- List of the errors:
  https://www.intel.com/content/www/us/en/docs/trace-analyzer-collector/user-guide-reference/2021-10/supported-errors.html
- Whole guide as a PDF: https://cdrdv2.intel.com/v1/dl/getContent/784740

Their status in SMPI:

- **Covered**: `LOCAL:EXIT:SIGNAL`, `LOCAL:EXIT:BEFORE_MPI_FINALIZE`, `LOCAL:MPI:CALL_FAILED` (parameter checking),
  `LOCAL:REQUEST:ILLEGAL_CALL`, `LOCAL:REQUEST:PREMATURE_FREE`, `GLOBAL:MSG:PENDING`, and all the collective checks:
  `GLOBAL:COLLECTIVE:OPERATION_MISMATCH`, `SIZE_MISMATCH`, `REDUCTION_OPERATION_MISMATCH`, `ROOT_MISMATCH`,
  `INVALID_PARAMETER` and `COMM_FREE_MISMATCH`.
- **Partially covered**:
  - `LOCAL:MEMORY:OVERLAP`: identical send and receive buffers, and overlapping v-collective blocks, are reported;
    partial overlaps are M6 and concurrent operations M7.
  - `LOCAL:MEMORY:INACCESSIBLE`: NULL buffers and buffers smaller than the data are reported; freed or read-only memory
    is H5.
  - `LOCAL:REQUEST:NOT_FREED`, `LOCAL:DATATYPE:NOT_FREED`: leaks are listed at the end with `smpi/list-leaks`; the
    warning during the run is NP2.
  - `LOCAL:BUFFER:INSUFFICIENT_BUFFER`: each buffered message is checked; the messages in flight are M1.
  - `GLOBAL:MSG:DATATYPE:MISMATCH`, `GLOBAL:COLLECTIVE:DATATYPE:MISMATCH`: predefined datatypes and byte counts are
    compared; derived datatypes are H1.
  - `GLOBAL:DEADLOCK:HARD`: global deadlocks are reported in MPI terms; cycles while other ranks progress are H2.
  - `GLOBAL:DEADLOCK:POTENTIAL`: revealed with `smpi/send-is-detached-thresh:0`; other interleavings are H2.
- **Not covered**: `LOCAL:MEMORY:ILLEGAL_MODIFICATION` (M5), `LOCAL:MEMORY:ILLEGAL_ACCESS` (H6),
  `LOCAL:MEMORY:INITIALIZATION` (M4), `GLOBAL:MSG/COLLECTIVE:DATA_TRANSMISSION_CORRUPTED` (NP6),
  `GLOBAL:DEADLOCK:NO_PROGRESS` (NP7).

## MUST coverage

MUST 1.9 defines 121 message identifiers, in `modules/Common/MustEnums.h` of its sources. The official tarballs
(latest 1.11.2) and documentation were unreachable in November 2026, so the sources come from an artifact repository of
RWTH Aachen (file `MUST-v1.9-omp-checks.tar.gz`, commit e1b187e of 2024-09-28).

- Sources: https://github.com/RWTH-HPC/must-omp-clock-checks
- Official tarballs and documentation: https://hpc.rwth-aachen.de/must/files/
- Datatype checks: Hilbrich et al., Holistic Debugging of MPI Derived Datatypes, https://www.osti.gov/servlets/purl/1107310

Their status in SMPI:

- **Covered**: the integer checks (negative values, ranks and roots out of range, duplicated ranks in groups, tags
  above 32767, Cartesian topologies larger than the communicator), the null, unknown or freed handles, uncommitted
  datatypes, `MPI_IN_PLACE` misuse, NULL pointers, active requests started again, leaks (with `smpi/list-leaks`), lost
  messages, collective call, root and operation mismatches, mixed blocking and nonblocking collectives, count arrays
  that differ between ranks, buffered sends without buffer, freeing a pending receive, and `MPI_BYTE` messages ending
  in the middle of an element.
- **Partially covered**: type matching of derived datatypes (`TYPEMATCH_*`, H1), overlaps (`OVERLAPPED_SEND`,
  `OVERLAPPED_RECV`, `SELFOVERLAPPED`: M6, M7, M8), deadlocks (`DEADLOCK`, H2), space of the buffered sends
  (`BUFFER_OUTSIZED`, M1).
- **Not covered**: thread level (`THREADLEVEL`, E10), struct alignment (`DATATYPE_BAD_ALIGNMENT`, M9), buffer types
  with TypeART (`TYPEMATCH_ALIGNMENT`, H7), RMA races (H4), races with threads and OpenMP (`DATARACE`,
  `MPI_MULTIPLE_THREADS`, `OPENMP`: NP4), partitioned communication (NP5), and the warnings on legal arguments
  (`INTEGER_ZERO`, `INTEGER_NOT_ONE_OR_ZERO`, `DATATYPE_COMMITED`, ...: NP1, NP3).
