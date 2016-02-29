#ifndef SMPI_PRIVATE_HPP
#define SMPI_PRIVATE_HPP

#ifdef HAVE_PAPI
typedef 
    std::vector<std::pair</* counter name */std::string, /* counter value */long long>> papi_counter_t;
XBT_PRIVATE papi_counter_t& smpi_process_counter_data(void);
XBT_PRIVATE int smpi_process_event_set(void);
#endif

#endif
