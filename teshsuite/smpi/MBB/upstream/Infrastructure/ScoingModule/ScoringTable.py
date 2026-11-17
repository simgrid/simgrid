import pandas as pd
import numpy as np

from tqdm.auto import tqdm

tqdm.pandas()

from Infrastructure.MPIAPIInfo.MPIAPICategories import mpi_all_mpi

# Settings
categories_to_score = ["RANK", "TAG", "POLYXFER_NUM_ELEM_NNI", "DATATYPE", "COMMUNICATOR", "OPERATION"]

columns_to_lowercase = ["call", "DATATYPE", "COMMUNICATOR"]

# maximum total score
maximum_score = 100
# maximum score per call
maximum_call_score = 1

# calls to completely ignore while scoring
# ALL Programs must use MPI init, using it for scoring is therefore not that useful
calls_to_exclude = ["MPI_Comm_rank", "MPI_Comm_size", "MPI_Init", "MPI_Finalize", "MPI_Init_thread"]

# end settings
max_call_weight = float(maximum_score) / float(maximum_call_score)


# replace value with other to use as wildcard
def clean_data_to_score(row, categories_to_score):
    for c in categories_to_score:
        cc = c + "_CATEGORY"
        if not pd.isna(row[c]):
            if row[cc] not in ["MPI_constant", "literal_constant", "handle"]:
                row[c] = "other"
            if row[c] == "inconclusive":
                row[c] = "other"
            if c == "TAG" and row[c] != "MPI_ANY_TAG":
                # discard all tag values besides the one with special meanings
                row[c] = "other"
    return row


def get_scoring_table(df, c_only=False):
    df = df[~df["call"].isin(calls_to_exclude)]
    if c_only:
        # remove fortran
        df = df[~df['src_location'].str.contains(".f", regex=False)]
        df = df[~df['src_location'].str.contains(".F", regex=False)]
    columns = ["call"] + categories_to_score + ["score"]
    num_categories = len(categories_to_score)
    types = [pd.StringDtype] + [pd.StringDtype for _ in range(num_categories)] + [np.float64]

    scoring_table = pd.DataFrame(columns=columns)

    call_weights = {}
    # other ist ein wildcard, der alles matcht

    # there may also be openmp, cuda etc. in the data
    df_only_mpi = df[df["call"].isin(mpi_all_mpi)].apply(clean_data_to_score, args=(categories_to_score,), axis=1)
    total_calls = float(len(df_only_mpi))

    for call, count in df_only_mpi["call"].value_counts().items():
        call_weights[call] = (float(count) / total_calls) * max_call_weight

    for call, this_call_weight in tqdm(call_weights.items()):
        if num_categories == 0:
            scoring_table.loc[len(scoring_table)] = [call, this_call_weight]
        else:
            this_call_only = df_only_mpi[df_only_mpi["call"] == call][categories_to_score]
            unique_calls_to_this = (
                # drops all NA but disregards the cols where all values are NA (e.g. the param is not used in this call
                # but all other NA values (wrong reading of calls) are discarded
                this_call_only.dropna(subset=this_call_only.columns[~this_call_only.isnull().all()], how='any')
                .groupby(categories_to_score, as_index=False, dropna=False).size()
            )
            total_calls_to_this_mpi_func = unique_calls_to_this['size'].sum()

            for _, row in unique_calls_to_this.iterrows():
                score = this_call_weight * float(maximum_call_score) * float(row['size']) / float(
                    total_calls_to_this_mpi_func)
                scoring_table.loc[len(scoring_table)] = [call] + row[categories_to_score].tolist() + [score]
    for c in columns_to_lowercase:
        scoring_table[c] = scoring_table[c].str.lower()

    scoring_table.rename({"POLYXFER_NUM_ELEM_NNI": "COUNT"}, inplace=True)

    return scoring_table

def is_combination_important(real_world_score_table, call,
                             rank=None, tag=None, count=None, datatype=None, communicator=None, op=None):
    # the scoreboard has other values == wildcard, we treat the given standard values as matching
    standard_rank = 0
    standard_tag = 0
    standard_count = 1
    standard_datatype = "MPI_INT".lower()
    standard_communicator = "MPI_COMM_WORLD".lower()
    standard_op = "MPI_SUM"

    # Filter based on the 'call' column
    relevant = real_world_score_table[real_world_score_table["call"] == call]

    # Filter by 'rank'
    if rank is not None:
        if rank == standard_rank:
            relevant = relevant[(relevant["RANK"] == rank) | (relevant["RANK"] == "other")]
        else:
            relevant = relevant[relevant["RANK"] == rank]

    # Filter by 'tag'
    if tag is not None:
        if tag == standard_tag:
            relevant = relevant[(relevant["TAG"] == tag) | (relevant["TAG"] == "other")]
        else:
            relevant = relevant[relevant["TAG"] == tag]

    # Filter by 'count'
    if count is not None:
        if count == standard_count:
            relevant = relevant[(relevant["COUNT"] == count) | (relevant["COUNT"] == "other")]
        else:
            relevant = relevant[relevant["COUNT"] == count]

    # Filter by 'datatype'
    if datatype is not None:
        if datatype.lower() == standard_datatype:
            relevant = relevant[
                (relevant["DATATYPE"] == datatype) | (relevant["DATATYPE"] == "other")]
        else:
            relevant = relevant[relevant["DATATYPE"] == datatype]

    # Filter by 'communicator'
    if communicator is not None:
        if communicator.lower() == standard_communicator:
            relevant = relevant[
                (relevant["COMMUNICATOR"] == communicator) | (relevant["COMMUNICATOR"] == "other")]
        else:
            relevant = relevant[relevant["COMMUNICATOR"] == communicator]

    # Filter by 'op'
    if op is not None:
        if op == standard_op:
            relevant = relevant[(relevant["OP"] == op) | (relevant["OP"] == "other")]
        else:
            relevant = relevant[relevant["OP"] == op]

    return len(relevant) > 0
