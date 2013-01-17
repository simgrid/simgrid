#define main smpi_simulated_main__(int argc, char **argv);\
int main(int argc, char **argv){\
MAIN__(&smpi_simulated_main__,&argc,argv);\
return 0;\
}\
int smpi_simulated_main__
