/* This file is perl-generated, of course */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(structs,test,"Logs about the gigantic struct test");

#define READ  0
#define WRITE 1
#define RW    2

void write_read(const char *type,void *src, void *dst, gras_socket_t *sock, int direction);

GRAS_DEFINE_TYPE(cccc,struct cccc { char a; char b; char c;char d;};)
GRAS_DEFINE_TYPE(ccsc,struct ccsc { char a; char b; short int c;char d;};)
GRAS_DEFINE_TYPE(ccic,struct ccic { char a; char b; int c;char d;};)
GRAS_DEFINE_TYPE(cclc,struct cclc { char a; char b; long int c;char d;};)
GRAS_DEFINE_TYPE(ccLc,struct ccLc { char a; char b; long long int c;char d;};)
GRAS_DEFINE_TYPE(ccfc,struct ccfc { char a; char b; float c;char d;};)
GRAS_DEFINE_TYPE(ccdc,struct ccdc { char a; char b; double c;char d;};)
GRAS_DEFINE_TYPE(sccc,struct sccc { short int a; char b; char c;char d;};)
GRAS_DEFINE_TYPE(scsc,struct scsc { short int a; char b; short int c;char d;};)
GRAS_DEFINE_TYPE(scic,struct scic { short int a; char b; int c;char d;};)
GRAS_DEFINE_TYPE(sclc,struct sclc { short int a; char b; long int c;char d;};)
GRAS_DEFINE_TYPE(scLc,struct scLc { short int a; char b; long long int c;char d;};)
GRAS_DEFINE_TYPE(scfc,struct scfc { short int a; char b; float c;char d;};)
GRAS_DEFINE_TYPE(scdc,struct scdc { short int a; char b; double c;char d;};)
GRAS_DEFINE_TYPE(iccc,struct iccc { int a; char b; char c;char d;};)
GRAS_DEFINE_TYPE(icsc,struct icsc { int a; char b; short int c;char d;};)
GRAS_DEFINE_TYPE(icic,struct icic { int a; char b; int c;char d;};)
GRAS_DEFINE_TYPE(iclc,struct iclc { int a; char b; long int c;char d;};)
GRAS_DEFINE_TYPE(icLc,struct icLc { int a; char b; long long int c;char d;};)
GRAS_DEFINE_TYPE(icfc,struct icfc { int a; char b; float c;char d;};)
GRAS_DEFINE_TYPE(icdc,struct icdc { int a; char b; double c;char d;};)
GRAS_DEFINE_TYPE(lccc,struct lccc { long int a; char b; char c;char d;};)
GRAS_DEFINE_TYPE(lcsc,struct lcsc { long int a; char b; short int c;char d;};)
GRAS_DEFINE_TYPE(lcic,struct lcic { long int a; char b; int c;char d;};)
GRAS_DEFINE_TYPE(lclc,struct lclc { long int a; char b; long int c;char d;};)
GRAS_DEFINE_TYPE(lcLc,struct lcLc { long int a; char b; long long int c;char d;};)
GRAS_DEFINE_TYPE(lcfc,struct lcfc { long int a; char b; float c;char d;};)
GRAS_DEFINE_TYPE(lcdc,struct lcdc { long int a; char b; double c;char d;};)
GRAS_DEFINE_TYPE(Lccc,struct Lccc { long long int a; char b; char c;char d;};)
GRAS_DEFINE_TYPE(Lcsc,struct Lcsc { long long int a; char b; short int c;char d;};)
GRAS_DEFINE_TYPE(Lcic,struct Lcic { long long int a; char b; int c;char d;};)
GRAS_DEFINE_TYPE(Lclc,struct Lclc { long long int a; char b; long int c;char d;};)
GRAS_DEFINE_TYPE(LcLc,struct LcLc { long long int a; char b; long long int c;char d;};)
GRAS_DEFINE_TYPE(Lcfc,struct Lcfc { long long int a; char b; float c;char d;};)
GRAS_DEFINE_TYPE(Lcdc,struct Lcdc { long long int a; char b; double c;char d;};)
GRAS_DEFINE_TYPE(fccc,struct fccc { float a; char b; char c;char d;};)
GRAS_DEFINE_TYPE(fcsc,struct fcsc { float a; char b; short int c;char d;};)
GRAS_DEFINE_TYPE(fcic,struct fcic { float a; char b; int c;char d;};)
GRAS_DEFINE_TYPE(fclc,struct fclc { float a; char b; long int c;char d;};)
GRAS_DEFINE_TYPE(fcLc,struct fcLc { float a; char b; long long int c;char d;};)
GRAS_DEFINE_TYPE(fcfc,struct fcfc { float a; char b; float c;char d;};)
GRAS_DEFINE_TYPE(fcdc,struct fcdc { float a; char b; double c;char d;};)
GRAS_DEFINE_TYPE(dccc,struct dccc { double a; char b; char c;char d;};)
GRAS_DEFINE_TYPE(dcsc,struct dcsc { double a; char b; short int c;char d;};)
GRAS_DEFINE_TYPE(dcic,struct dcic { double a; char b; int c;char d;};)
GRAS_DEFINE_TYPE(dclc,struct dclc { double a; char b; long int c;char d;};)
GRAS_DEFINE_TYPE(dcLc,struct dcLc { double a; char b; long long int c;char d;};)
GRAS_DEFINE_TYPE(dcfc,struct dcfc { double a; char b; float c;char d;};)
GRAS_DEFINE_TYPE(dcdc,struct dcdc { double a; char b; double c;char d;};)

#define test(a) xbt_assert(a)
void register_structures(void);
void register_structures(void) {
  gras_msgtype_declare("cccc", gras_datadesc_by_symbol(cccc));
  gras_msgtype_declare("ccsc", gras_datadesc_by_symbol(ccsc));
  gras_msgtype_declare("ccic", gras_datadesc_by_symbol(ccic));
  gras_msgtype_declare("cclc", gras_datadesc_by_symbol(cclc));
  gras_msgtype_declare("ccLc", gras_datadesc_by_symbol(ccLc));
  gras_msgtype_declare("ccfc", gras_datadesc_by_symbol(ccfc));
  gras_msgtype_declare("ccdc", gras_datadesc_by_symbol(ccdc));
  gras_msgtype_declare("sccc", gras_datadesc_by_symbol(sccc));
  gras_msgtype_declare("scsc", gras_datadesc_by_symbol(scsc));
  gras_msgtype_declare("scic", gras_datadesc_by_symbol(scic));
  gras_msgtype_declare("sclc", gras_datadesc_by_symbol(sclc));
  gras_msgtype_declare("scLc", gras_datadesc_by_symbol(scLc));
  gras_msgtype_declare("scfc", gras_datadesc_by_symbol(scfc));
  gras_msgtype_declare("scdc", gras_datadesc_by_symbol(scdc));
  gras_msgtype_declare("iccc", gras_datadesc_by_symbol(iccc));
  gras_msgtype_declare("icsc", gras_datadesc_by_symbol(icsc));
  gras_msgtype_declare("icic", gras_datadesc_by_symbol(icic));
  gras_msgtype_declare("iclc", gras_datadesc_by_symbol(iclc));
  gras_msgtype_declare("icLc", gras_datadesc_by_symbol(icLc));
  gras_msgtype_declare("icfc", gras_datadesc_by_symbol(icfc));
  gras_msgtype_declare("icdc", gras_datadesc_by_symbol(icdc));
  gras_msgtype_declare("lccc", gras_datadesc_by_symbol(lccc));
  gras_msgtype_declare("lcsc", gras_datadesc_by_symbol(lcsc));
  gras_msgtype_declare("lcic", gras_datadesc_by_symbol(lcic));
  gras_msgtype_declare("lclc", gras_datadesc_by_symbol(lclc));
  gras_msgtype_declare("lcLc", gras_datadesc_by_symbol(lcLc));
  gras_msgtype_declare("lcfc", gras_datadesc_by_symbol(lcfc));
  gras_msgtype_declare("lcdc", gras_datadesc_by_symbol(lcdc));
  gras_msgtype_declare("Lccc", gras_datadesc_by_symbol(Lccc));
  gras_msgtype_declare("Lcsc", gras_datadesc_by_symbol(Lcsc));
  gras_msgtype_declare("Lcic", gras_datadesc_by_symbol(Lcic));
  gras_msgtype_declare("Lclc", gras_datadesc_by_symbol(Lclc));
  gras_msgtype_declare("LcLc", gras_datadesc_by_symbol(LcLc));
  gras_msgtype_declare("Lcfc", gras_datadesc_by_symbol(Lcfc));
  gras_msgtype_declare("Lcdc", gras_datadesc_by_symbol(Lcdc));
  gras_msgtype_declare("fccc", gras_datadesc_by_symbol(fccc));
  gras_msgtype_declare("fcsc", gras_datadesc_by_symbol(fcsc));
  gras_msgtype_declare("fcic", gras_datadesc_by_symbol(fcic));
  gras_msgtype_declare("fclc", gras_datadesc_by_symbol(fclc));
  gras_msgtype_declare("fcLc", gras_datadesc_by_symbol(fcLc));
  gras_msgtype_declare("fcfc", gras_datadesc_by_symbol(fcfc));
  gras_msgtype_declare("fcdc", gras_datadesc_by_symbol(fcdc));
  gras_msgtype_declare("dccc", gras_datadesc_by_symbol(dccc));
  gras_msgtype_declare("dcsc", gras_datadesc_by_symbol(dcsc));
  gras_msgtype_declare("dcic", gras_datadesc_by_symbol(dcic));
  gras_msgtype_declare("dclc", gras_datadesc_by_symbol(dclc));
  gras_msgtype_declare("dcLc", gras_datadesc_by_symbol(dcLc));
  gras_msgtype_declare("dcfc", gras_datadesc_by_symbol(dcfc));
  gras_msgtype_declare("dcdc", gras_datadesc_by_symbol(dcdc));
}
void test_structures(gras_socket_t *sock, int direction);
void test_structures(gras_socket_t *sock, int direction) {
  struct cccc my_cccc = {'w'+(char)1,'w'+(char)2,'w'+(char)3,'w'+(char)4}, my_cccc2;
  struct ccsc my_ccsc = {'w'+(char)1,'w'+(char)2,134+(short int)3,'w'+(char)4}, my_ccsc2;
  struct ccic my_ccic = {'w'+(char)1,'w'+(char)2,-11249+(int)3,'w'+(char)4}, my_ccic2;
  struct cclc my_cclc = {'w'+(char)1,'w'+(char)2,31319919+(long int)3,'w'+(char)4}, my_cclc2;
  struct ccLc my_ccLc = {'w'+(char)1,'w'+(char)2,-232130010+(long long int)3,'w'+(char)4}, my_ccLc2;
  struct ccfc my_ccfc = {'w'+(char)1,'w'+(char)2,-11313.1135+(float)3,'w'+(char)4}, my_ccfc2;
  struct ccdc my_ccdc = {'w'+(char)1,'w'+(char)2,1424420.11331+(double)3,'w'+(char)4}, my_ccdc2;
  struct sccc my_sccc = {134+(short int)1,'w'+(char)2,'w'+(char)3,'w'+(char)4}, my_sccc2;
  struct scsc my_scsc = {134+(short int)1,'w'+(char)2,134+(short int)3,'w'+(char)4}, my_scsc2;
  struct scic my_scic = {134+(short int)1,'w'+(char)2,-11249+(int)3,'w'+(char)4}, my_scic2;
  struct sclc my_sclc = {134+(short int)1,'w'+(char)2,31319919+(long int)3,'w'+(char)4}, my_sclc2;
  struct scLc my_scLc = {134+(short int)1,'w'+(char)2,-232130010+(long long int)3,'w'+(char)4}, my_scLc2;
  struct scfc my_scfc = {134+(short int)1,'w'+(char)2,-11313.1135+(float)3,'w'+(char)4}, my_scfc2;
  struct scdc my_scdc = {134+(short int)1,'w'+(char)2,1424420.11331+(double)3,'w'+(char)4}, my_scdc2;
  struct iccc my_iccc = {-11249+(int)1,'w'+(char)2,'w'+(char)3,'w'+(char)4}, my_iccc2;
  struct icsc my_icsc = {-11249+(int)1,'w'+(char)2,134+(short int)3,'w'+(char)4}, my_icsc2;
  struct icic my_icic = {-11249+(int)1,'w'+(char)2,-11249+(int)3,'w'+(char)4}, my_icic2;
  struct iclc my_iclc = {-11249+(int)1,'w'+(char)2,31319919+(long int)3,'w'+(char)4}, my_iclc2;
  struct icLc my_icLc = {-11249+(int)1,'w'+(char)2,-232130010+(long long int)3,'w'+(char)4}, my_icLc2;
  struct icfc my_icfc = {-11249+(int)1,'w'+(char)2,-11313.1135+(float)3,'w'+(char)4}, my_icfc2;
  struct icdc my_icdc = {-11249+(int)1,'w'+(char)2,1424420.11331+(double)3,'w'+(char)4}, my_icdc2;
  struct lccc my_lccc = {31319919+(long int)1,'w'+(char)2,'w'+(char)3,'w'+(char)4}, my_lccc2;
  struct lcsc my_lcsc = {31319919+(long int)1,'w'+(char)2,134+(short int)3,'w'+(char)4}, my_lcsc2;
  struct lcic my_lcic = {31319919+(long int)1,'w'+(char)2,-11249+(int)3,'w'+(char)4}, my_lcic2;
  struct lclc my_lclc = {31319919+(long int)1,'w'+(char)2,31319919+(long int)3,'w'+(char)4}, my_lclc2;
  struct lcLc my_lcLc = {31319919+(long int)1,'w'+(char)2,-232130010+(long long int)3,'w'+(char)4}, my_lcLc2;
  struct lcfc my_lcfc = {31319919+(long int)1,'w'+(char)2,-11313.1135+(float)3,'w'+(char)4}, my_lcfc2;
  struct lcdc my_lcdc = {31319919+(long int)1,'w'+(char)2,1424420.11331+(double)3,'w'+(char)4}, my_lcdc2;
  struct Lccc my_Lccc = {-232130010+(long long int)1,'w'+(char)2,'w'+(char)3,'w'+(char)4}, my_Lccc2;
  struct Lcsc my_Lcsc = {-232130010+(long long int)1,'w'+(char)2,134+(short int)3,'w'+(char)4}, my_Lcsc2;
  struct Lcic my_Lcic = {-232130010+(long long int)1,'w'+(char)2,-11249+(int)3,'w'+(char)4}, my_Lcic2;
  struct Lclc my_Lclc = {-232130010+(long long int)1,'w'+(char)2,31319919+(long int)3,'w'+(char)4}, my_Lclc2;
  struct LcLc my_LcLc = {-232130010+(long long int)1,'w'+(char)2,-232130010+(long long int)3,'w'+(char)4}, my_LcLc2;
  struct Lcfc my_Lcfc = {-232130010+(long long int)1,'w'+(char)2,-11313.1135+(float)3,'w'+(char)4}, my_Lcfc2;
  struct Lcdc my_Lcdc = {-232130010+(long long int)1,'w'+(char)2,1424420.11331+(double)3,'w'+(char)4}, my_Lcdc2;
  struct fccc my_fccc = {-11313.1135+(float)1,'w'+(char)2,'w'+(char)3,'w'+(char)4}, my_fccc2;
  struct fcsc my_fcsc = {-11313.1135+(float)1,'w'+(char)2,134+(short int)3,'w'+(char)4}, my_fcsc2;
  struct fcic my_fcic = {-11313.1135+(float)1,'w'+(char)2,-11249+(int)3,'w'+(char)4}, my_fcic2;
  struct fclc my_fclc = {-11313.1135+(float)1,'w'+(char)2,31319919+(long int)3,'w'+(char)4}, my_fclc2;
  struct fcLc my_fcLc = {-11313.1135+(float)1,'w'+(char)2,-232130010+(long long int)3,'w'+(char)4}, my_fcLc2;
  struct fcfc my_fcfc = {-11313.1135+(float)1,'w'+(char)2,-11313.1135+(float)3,'w'+(char)4}, my_fcfc2;
  struct fcdc my_fcdc = {-11313.1135+(float)1,'w'+(char)2,1424420.11331+(double)3,'w'+(char)4}, my_fcdc2;
  struct dccc my_dccc = {1424420.11331+(double)1,'w'+(char)2,'w'+(char)3,'w'+(char)4}, my_dccc2;
  struct dcsc my_dcsc = {1424420.11331+(double)1,'w'+(char)2,134+(short int)3,'w'+(char)4}, my_dcsc2;
  struct dcic my_dcic = {1424420.11331+(double)1,'w'+(char)2,-11249+(int)3,'w'+(char)4}, my_dcic2;
  struct dclc my_dclc = {1424420.11331+(double)1,'w'+(char)2,31319919+(long int)3,'w'+(char)4}, my_dclc2;
  struct dcLc my_dcLc = {1424420.11331+(double)1,'w'+(char)2,-232130010+(long long int)3,'w'+(char)4}, my_dcLc2;
  struct dcfc my_dcfc = {1424420.11331+(double)1,'w'+(char)2,-11313.1135+(float)3,'w'+(char)4}, my_dcfc2;
  struct dcdc my_dcdc = {1424420.11331+(double)1,'w'+(char)2,1424420.11331+(double)3,'w'+(char)4}, my_dcdc2;
  INFO0("---- Test on all possible struct having 3 fields (49 structs) ----");
  write_read("cccc", &my_cccc, &my_cccc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_cccc.a == my_cccc2.a);
     test(my_cccc.b == my_cccc2.b);
     test(my_cccc.c == my_cccc2.c);
     test(my_cccc.d == my_cccc2.d);
     if (!failed) VERB0("Passed cccc");
  }
  write_read("ccsc", &my_ccsc, &my_ccsc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_ccsc.a == my_ccsc2.a);
     test(my_ccsc.b == my_ccsc2.b);
     test(my_ccsc.c == my_ccsc2.c);
     test(my_ccsc.d == my_ccsc2.d);
     if (!failed) VERB0("Passed ccsc");
  }
  write_read("ccic", &my_ccic, &my_ccic2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_ccic.a == my_ccic2.a);
     test(my_ccic.b == my_ccic2.b);
     test(my_ccic.c == my_ccic2.c);
     test(my_ccic.d == my_ccic2.d);
     if (!failed) VERB0("Passed ccic");
  }
  write_read("cclc", &my_cclc, &my_cclc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_cclc.a == my_cclc2.a);
     test(my_cclc.b == my_cclc2.b);
     test(my_cclc.c == my_cclc2.c);
     test(my_cclc.d == my_cclc2.d);
     if (!failed) VERB0("Passed cclc");
  }
  write_read("ccLc", &my_ccLc, &my_ccLc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_ccLc.a == my_ccLc2.a);
     test(my_ccLc.b == my_ccLc2.b);
     test(my_ccLc.c == my_ccLc2.c);
     test(my_ccLc.d == my_ccLc2.d);
     if (!failed) VERB0("Passed ccLc");
  }
  write_read("ccfc", &my_ccfc, &my_ccfc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_ccfc.a == my_ccfc2.a);
     test(my_ccfc.b == my_ccfc2.b);
     test(my_ccfc.c == my_ccfc2.c);
     test(my_ccfc.d == my_ccfc2.d);
     if (!failed) VERB0("Passed ccfc");
  }
  write_read("ccdc", &my_ccdc, &my_ccdc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_ccdc.a == my_ccdc2.a);
     test(my_ccdc.b == my_ccdc2.b);
     test(my_ccdc.c == my_ccdc2.c);
     test(my_ccdc.d == my_ccdc2.d);
     if (!failed) VERB0("Passed ccdc");
  }
  write_read("sccc", &my_sccc, &my_sccc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_sccc.a == my_sccc2.a);
     test(my_sccc.b == my_sccc2.b);
     test(my_sccc.c == my_sccc2.c);
     test(my_sccc.d == my_sccc2.d);
     if (!failed) VERB0("Passed sccc");
  }
  write_read("scsc", &my_scsc, &my_scsc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_scsc.a == my_scsc2.a);
     test(my_scsc.b == my_scsc2.b);
     test(my_scsc.c == my_scsc2.c);
     test(my_scsc.d == my_scsc2.d);
     if (!failed) VERB0("Passed scsc");
  }
  write_read("scic", &my_scic, &my_scic2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_scic.a == my_scic2.a);
     test(my_scic.b == my_scic2.b);
     test(my_scic.c == my_scic2.c);
     test(my_scic.d == my_scic2.d);
     if (!failed) VERB0("Passed scic");
  }
  write_read("sclc", &my_sclc, &my_sclc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_sclc.a == my_sclc2.a);
     test(my_sclc.b == my_sclc2.b);
     test(my_sclc.c == my_sclc2.c);
     test(my_sclc.d == my_sclc2.d);
     if (!failed) VERB0("Passed sclc");
  }
  write_read("scLc", &my_scLc, &my_scLc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_scLc.a == my_scLc2.a);
     test(my_scLc.b == my_scLc2.b);
     test(my_scLc.c == my_scLc2.c);
     test(my_scLc.d == my_scLc2.d);
     if (!failed) VERB0("Passed scLc");
  }
  write_read("scfc", &my_scfc, &my_scfc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_scfc.a == my_scfc2.a);
     test(my_scfc.b == my_scfc2.b);
     test(my_scfc.c == my_scfc2.c);
     test(my_scfc.d == my_scfc2.d);
     if (!failed) VERB0("Passed scfc");
  }
  write_read("scdc", &my_scdc, &my_scdc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_scdc.a == my_scdc2.a);
     test(my_scdc.b == my_scdc2.b);
     test(my_scdc.c == my_scdc2.c);
     test(my_scdc.d == my_scdc2.d);
     if (!failed) VERB0("Passed scdc");
  }
  write_read("iccc", &my_iccc, &my_iccc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_iccc.a == my_iccc2.a);
     test(my_iccc.b == my_iccc2.b);
     test(my_iccc.c == my_iccc2.c);
     test(my_iccc.d == my_iccc2.d);
     if (!failed) VERB0("Passed iccc");
  }
  write_read("icsc", &my_icsc, &my_icsc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_icsc.a == my_icsc2.a);
     test(my_icsc.b == my_icsc2.b);
     test(my_icsc.c == my_icsc2.c);
     test(my_icsc.d == my_icsc2.d);
     if (!failed) VERB0("Passed icsc");
  }
  write_read("icic", &my_icic, &my_icic2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_icic.a == my_icic2.a);
     test(my_icic.b == my_icic2.b);
     test(my_icic.c == my_icic2.c);
     test(my_icic.d == my_icic2.d);
     if (!failed) VERB0("Passed icic");
  }
  write_read("iclc", &my_iclc, &my_iclc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_iclc.a == my_iclc2.a);
     test(my_iclc.b == my_iclc2.b);
     test(my_iclc.c == my_iclc2.c);
     test(my_iclc.d == my_iclc2.d);
     if (!failed) VERB0("Passed iclc");
  }
  write_read("icLc", &my_icLc, &my_icLc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_icLc.a == my_icLc2.a);
     test(my_icLc.b == my_icLc2.b);
     test(my_icLc.c == my_icLc2.c);
     test(my_icLc.d == my_icLc2.d);
     if (!failed) VERB0("Passed icLc");
  }
  write_read("icfc", &my_icfc, &my_icfc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_icfc.a == my_icfc2.a);
     test(my_icfc.b == my_icfc2.b);
     test(my_icfc.c == my_icfc2.c);
     test(my_icfc.d == my_icfc2.d);
     if (!failed) VERB0("Passed icfc");
  }
  write_read("icdc", &my_icdc, &my_icdc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_icdc.a == my_icdc2.a);
     test(my_icdc.b == my_icdc2.b);
     test(my_icdc.c == my_icdc2.c);
     test(my_icdc.d == my_icdc2.d);
     if (!failed) VERB0("Passed icdc");
  }
  write_read("lccc", &my_lccc, &my_lccc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_lccc.a == my_lccc2.a);
     test(my_lccc.b == my_lccc2.b);
     test(my_lccc.c == my_lccc2.c);
     test(my_lccc.d == my_lccc2.d);
     if (!failed) VERB0("Passed lccc");
  }
  write_read("lcsc", &my_lcsc, &my_lcsc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_lcsc.a == my_lcsc2.a);
     test(my_lcsc.b == my_lcsc2.b);
     test(my_lcsc.c == my_lcsc2.c);
     test(my_lcsc.d == my_lcsc2.d);
     if (!failed) VERB0("Passed lcsc");
  }
  write_read("lcic", &my_lcic, &my_lcic2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_lcic.a == my_lcic2.a);
     test(my_lcic.b == my_lcic2.b);
     test(my_lcic.c == my_lcic2.c);
     test(my_lcic.d == my_lcic2.d);
     if (!failed) VERB0("Passed lcic");
  }
  write_read("lclc", &my_lclc, &my_lclc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_lclc.a == my_lclc2.a);
     test(my_lclc.b == my_lclc2.b);
     test(my_lclc.c == my_lclc2.c);
     test(my_lclc.d == my_lclc2.d);
     if (!failed) VERB0("Passed lclc");
  }
  write_read("lcLc", &my_lcLc, &my_lcLc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_lcLc.a == my_lcLc2.a);
     test(my_lcLc.b == my_lcLc2.b);
     test(my_lcLc.c == my_lcLc2.c);
     test(my_lcLc.d == my_lcLc2.d);
     if (!failed) VERB0("Passed lcLc");
  }
  write_read("lcfc", &my_lcfc, &my_lcfc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_lcfc.a == my_lcfc2.a);
     test(my_lcfc.b == my_lcfc2.b);
     test(my_lcfc.c == my_lcfc2.c);
     test(my_lcfc.d == my_lcfc2.d);
     if (!failed) VERB0("Passed lcfc");
  }
  write_read("lcdc", &my_lcdc, &my_lcdc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_lcdc.a == my_lcdc2.a);
     test(my_lcdc.b == my_lcdc2.b);
     test(my_lcdc.c == my_lcdc2.c);
     test(my_lcdc.d == my_lcdc2.d);
     if (!failed) VERB0("Passed lcdc");
  }
  write_read("Lccc", &my_Lccc, &my_Lccc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_Lccc.a == my_Lccc2.a);
     test(my_Lccc.b == my_Lccc2.b);
     test(my_Lccc.c == my_Lccc2.c);
     test(my_Lccc.d == my_Lccc2.d);
     if (!failed) VERB0("Passed Lccc");
  }
  write_read("Lcsc", &my_Lcsc, &my_Lcsc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_Lcsc.a == my_Lcsc2.a);
     test(my_Lcsc.b == my_Lcsc2.b);
     test(my_Lcsc.c == my_Lcsc2.c);
     test(my_Lcsc.d == my_Lcsc2.d);
     if (!failed) VERB0("Passed Lcsc");
  }
  write_read("Lcic", &my_Lcic, &my_Lcic2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_Lcic.a == my_Lcic2.a);
     test(my_Lcic.b == my_Lcic2.b);
     test(my_Lcic.c == my_Lcic2.c);
     test(my_Lcic.d == my_Lcic2.d);
     if (!failed) VERB0("Passed Lcic");
  }
  write_read("Lclc", &my_Lclc, &my_Lclc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_Lclc.a == my_Lclc2.a);
     test(my_Lclc.b == my_Lclc2.b);
     test(my_Lclc.c == my_Lclc2.c);
     test(my_Lclc.d == my_Lclc2.d);
     if (!failed) VERB0("Passed Lclc");
  }
  write_read("LcLc", &my_LcLc, &my_LcLc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_LcLc.a == my_LcLc2.a);
     test(my_LcLc.b == my_LcLc2.b);
     test(my_LcLc.c == my_LcLc2.c);
     test(my_LcLc.d == my_LcLc2.d);
     if (!failed) VERB0("Passed LcLc");
  }
  write_read("Lcfc", &my_Lcfc, &my_Lcfc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_Lcfc.a == my_Lcfc2.a);
     test(my_Lcfc.b == my_Lcfc2.b);
     test(my_Lcfc.c == my_Lcfc2.c);
     test(my_Lcfc.d == my_Lcfc2.d);
     if (!failed) VERB0("Passed Lcfc");
  }
  write_read("Lcdc", &my_Lcdc, &my_Lcdc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_Lcdc.a == my_Lcdc2.a);
     test(my_Lcdc.b == my_Lcdc2.b);
     test(my_Lcdc.c == my_Lcdc2.c);
     test(my_Lcdc.d == my_Lcdc2.d);
     if (!failed) VERB0("Passed Lcdc");
  }
  write_read("fccc", &my_fccc, &my_fccc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_fccc.a == my_fccc2.a);
     test(my_fccc.b == my_fccc2.b);
     test(my_fccc.c == my_fccc2.c);
     test(my_fccc.d == my_fccc2.d);
     if (!failed) VERB0("Passed fccc");
  }
  write_read("fcsc", &my_fcsc, &my_fcsc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_fcsc.a == my_fcsc2.a);
     test(my_fcsc.b == my_fcsc2.b);
     test(my_fcsc.c == my_fcsc2.c);
     test(my_fcsc.d == my_fcsc2.d);
     if (!failed) VERB0("Passed fcsc");
  }
  write_read("fcic", &my_fcic, &my_fcic2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_fcic.a == my_fcic2.a);
     test(my_fcic.b == my_fcic2.b);
     test(my_fcic.c == my_fcic2.c);
     test(my_fcic.d == my_fcic2.d);
     if (!failed) VERB0("Passed fcic");
  }
  write_read("fclc", &my_fclc, &my_fclc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_fclc.a == my_fclc2.a);
     test(my_fclc.b == my_fclc2.b);
     test(my_fclc.c == my_fclc2.c);
     test(my_fclc.d == my_fclc2.d);
     if (!failed) VERB0("Passed fclc");
  }
  write_read("fcLc", &my_fcLc, &my_fcLc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_fcLc.a == my_fcLc2.a);
     test(my_fcLc.b == my_fcLc2.b);
     test(my_fcLc.c == my_fcLc2.c);
     test(my_fcLc.d == my_fcLc2.d);
     if (!failed) VERB0("Passed fcLc");
  }
  write_read("fcfc", &my_fcfc, &my_fcfc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_fcfc.a == my_fcfc2.a);
     test(my_fcfc.b == my_fcfc2.b);
     test(my_fcfc.c == my_fcfc2.c);
     test(my_fcfc.d == my_fcfc2.d);
     if (!failed) VERB0("Passed fcfc");
  }
  write_read("fcdc", &my_fcdc, &my_fcdc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_fcdc.a == my_fcdc2.a);
     test(my_fcdc.b == my_fcdc2.b);
     test(my_fcdc.c == my_fcdc2.c);
     test(my_fcdc.d == my_fcdc2.d);
     if (!failed) VERB0("Passed fcdc");
  }
  write_read("dccc", &my_dccc, &my_dccc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_dccc.a == my_dccc2.a);
     test(my_dccc.b == my_dccc2.b);
     test(my_dccc.c == my_dccc2.c);
     test(my_dccc.d == my_dccc2.d);
     if (!failed) VERB0("Passed dccc");
  }
  write_read("dcsc", &my_dcsc, &my_dcsc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_dcsc.a == my_dcsc2.a);
     test(my_dcsc.b == my_dcsc2.b);
     test(my_dcsc.c == my_dcsc2.c);
     test(my_dcsc.d == my_dcsc2.d);
     if (!failed) VERB0("Passed dcsc");
  }
  write_read("dcic", &my_dcic, &my_dcic2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_dcic.a == my_dcic2.a);
     test(my_dcic.b == my_dcic2.b);
     test(my_dcic.c == my_dcic2.c);
     test(my_dcic.d == my_dcic2.d);
     if (!failed) VERB0("Passed dcic");
  }
  write_read("dclc", &my_dclc, &my_dclc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_dclc.a == my_dclc2.a);
     test(my_dclc.b == my_dclc2.b);
     test(my_dclc.c == my_dclc2.c);
     test(my_dclc.d == my_dclc2.d);
     if (!failed) VERB0("Passed dclc");
  }
  write_read("dcLc", &my_dcLc, &my_dcLc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_dcLc.a == my_dcLc2.a);
     test(my_dcLc.b == my_dcLc2.b);
     test(my_dcLc.c == my_dcLc2.c);
     test(my_dcLc.d == my_dcLc2.d);
     if (!failed) VERB0("Passed dcLc");
  }
  write_read("dcfc", &my_dcfc, &my_dcfc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_dcfc.a == my_dcfc2.a);
     test(my_dcfc.b == my_dcfc2.b);
     test(my_dcfc.c == my_dcfc2.c);
     test(my_dcfc.d == my_dcfc2.d);
     if (!failed) VERB0("Passed dcfc");
  }
  write_read("dcdc", &my_dcdc, &my_dcdc2, sock,direction);
  if (direction == READ || direction == RW) {
     int failed = 0;
     test(my_dcdc.a == my_dcdc2.a);
     test(my_dcdc.b == my_dcdc2.b);
     test(my_dcdc.c == my_dcdc2.c);
     test(my_dcdc.d == my_dcdc2.d);
     if (!failed) VERB0("Passed dcdc");
  }
}
