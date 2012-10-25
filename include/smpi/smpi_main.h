#define main smpi_simulated_main(int argc, char **argv);\
int main(int argc, char **argv){\
MAIN__(&smpi_simulated_main,&argc,argv);\
return 0;\
}\
int smpi_simulated_main
