/* $Id$ */

#include <sys/types.h>  /* time_t */

/* linux defines this in /usr/include/values.h as 1.79769313486231570e+308 */
#ifndef MAXDOUBLE
#define MAXDOUBLE (999999999999.0)
#endif
/* #define HISTORYSIZE 200*/
/* reduce memory footprint */
#define HISTORYSIZE 100
#define MAX_MED_N HISTORYSIZE
#define MIN_DATA_HISTORY 10

#define PREDS 14
#define STATIC_PREDS (PREDS - 6)
#define SECOND_ORDER_PREDS (PREDS - 2)
#define MSE_PRED (STATIC_PREDS)
#define MAE_PRED (STATIC_PREDS+1)
#define WIN_MSE_PRED (STATIC_PREDS+2)
#define WIN_MAE_PRED (STATIC_PREDS+3)
#define TOT_MSE_PRED (SECOND_ORDER_PREDS)
#define TOT_MAE_PRED (SECOND_ORDER_PREDS+1)

#define DYNPRED TOT_MSE_PRED

#define LIFETIMES 120

#define MED_N 21
#define MED_ALPHA (0.1)

#define ERR_WIN 10

#define STATES 2
#define DIM 2
#define HMAX 10.0
#define HMIN 0.0

/*
 * for adaptive window predictors
 */
#define WIN_MIN 5
#define WIN_MAX 50
#define WIN_INIT 25

/*
 * for AR predictor
 */
#define AR_N 50
#define AR_P 20

struct pred_el
{
	float pred;
	double error;
	double sq_error;
	int medwin;
	int meanwin;
};

struct value_el
{
	time_t time;
	float value;
	struct pred_el preds[PREDS];
};

struct thruput_pred
{
	int head;
	int tail;
	struct value_el data[HISTORYSIZE];

	double mean;
	double run_mean;
	double total;

	double mses[PREDS];
	double mpes[PREDS];
	double counts[PREDS];
	int pred_nums[PREDS-STATIC_PREDS];
};

#define MODPLUS(a,b,m) (((a) + (b)) % (m))
#define MODMINUS(a,b,m) (((a) - (b) + (m)) % (m))

#define PRED(n,v) ((v).preds[(n)].pred)
#define ERR(n,v) ((v).preds[(n)].error)
#define SQERR(n,v) ((v).preds[(n)].sq_error)
#define MedWIN(n,v) ((v).preds[(n)].medwin)
#define MeanWIN(n,v) ((v).preds[(n)].meanwin)

#define TPPREDNUM(tp,n) ((tp)->pred_nums[(n)-STATIC_PREDS])
#define TPMSE(tp,n) ((tp)->mses[(n)])
#define TPMAE(tp,n) ((tp)->mpes[(n)])
#define TPCOUNT(tp,n) ((tp)->counts[(n)])

#define CURRVAL(tp) ((tp).data[MODMINUS((tp).head,1,HISTORYSIZE)].value)
#define CURRTIME(tp) ((tp).data[MODMINUS((tp).head,1,HISTORYSIZE)].time)
#define CURRPRED(tp) \
	PRED(TOT_MSE_PRED,(tp).data[MODMINUS((tp).head,1,HISTORYSIZE)])
#define CURRMSEPRED(tp) \
	PRED(TOT_MSE_PRED,(tp).data[MODMINUS((tp).head,1,HISTORYSIZE)])
#define CURRMAEPRED(tp) \
	PRED(TOT_MAE_PRED,(tp).data[MODMINUS((tp).head,1,HISTORYSIZE)])
#define CURRMSE(tp) \
        (TPCOUNT(&(tp),TOT_MSE_PRED) != 0.0 ?\
	(TPMSE(&(tp),TOT_MSE_PRED)/TPCOUNT(&(tp),TOT_MSE_PRED)) : 0.0)
#define CURRMAE(tp) \
        (TPCOUNT(&(tp),TOT_MAE_PRED) != 0.0 ?\
	(TPMAE(&(tp),TOT_MAE_PRED)/TPCOUNT(&(tp),TOT_MAE_PRED)) : 0.0)
#define CURRMSEPREDNUM(tp) TPPREDNUM(&(tp),TOT_MSE_PRED)
#define CURRMAEPREDNUM(tp) TPPREDNUM(&(tp),TOT_MAE_PRED)


void
InitThruputPrediction(struct thruput_pred *tp);


void
UpdateThruputPrediction(struct thruput_pred *tp,
                        float value,
                        time_t update_time);

#define GAIN (0.05)
#define LN_GAIN (0.9)

#define EL(a,i,j,n) ((a)[(i)*(n)+(j)])
