#ifndef INSTR_SMPI_H_
#define INSTR_SMPI_H_ 
#ifdef __cplusplus
extern "C" {
#endif

typedef struct smpi_trace_call_location {
  const char* filename;
  int linenumber;

  const char* previous_filename;
  int previous_linenumber;
} smpi_trace_call_location_t;

smpi_trace_call_location_t* smpi_trace_get_call_location();

#ifdef __cplusplus
}
#endif
#endif
