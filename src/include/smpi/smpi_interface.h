/********** Tracing **********/
/* from smpi_instr.c */
void TRACE_smpi_alloc(void);
void TRACE_smpi_release(void);
void TRACE_smpi_ptp_in(int rank, int src, int dst, const char *operation);
void TRACE_smpi_ptp_out(int rank, int src, int dst, const char *operation);
void TRACE_smpi_send(int rank, int src, int dst);
void TRACE_smpi_recv(int rank, int src, int dst);
void TRACE_smpi_init(int rank);
void TRACE_smpi_finalize(int rank);
