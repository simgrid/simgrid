#ifndef INSTR_SMPI_H_
#define INSTR_SMPI_H_ 
#ifdef __cplusplus
#include <string>
extern "C" {
#endif

typedef struct smpi_trace_call_location {
  const char* filename;
  int linenumber;

  const char* previous_filename;
  int previous_linenumber;

#ifdef __cplusplus
  std::string get_composed_key() {
    return std::string(previous_filename) + ':' + std::to_string(previous_linenumber) + ':' + filename + ':' + std::to_string(linenumber);
  }
#endif

} smpi_trace_call_location_t;

#ifdef __cplusplus
}
#endif
#endif
