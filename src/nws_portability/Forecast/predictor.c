/* $Id$ */

#include "config_portability.h"
#include <stdio.h>      /* fprintf() fflush() */
#include "strutil.h"    /* SAFESTRCPY() */
#include "predictor.h"

/* These should be in a header file, median.h */
extern double Median();
extern double TrimmedMedian();
extern void AdjustedMean();
extern void AdjustedMedian();
extern char *DefineHistogram();
extern char *DefineHistogramPred();
extern void PrintHistogram();
extern void ModalDev();

/* not yet
char *Histogram;
char *MeanPred;
char *MedianPred;
char *MiddlePred;
int Histodim;
int Histostates;
double Histomax;
double Histomin;
*/

double Onedev_mse;
double Onedev_mpe;
double Onedev_modal;
double Twodev_mse;
double Twodev_mpe;
double Twodev_modal;
double Devcounts;

double Lifetime_mse[LIFETIMES];
double Lifetime_mpe[LIFETIMES];
int Lifetime_count = 0;
extern int Median_sorted;

const char *Names[PREDS];
#define NAME(i) (Names[i])

void
PrintPredData(tp,where)
struct thruput_pred *tp;
int where;
{
	struct value_el *data = tp->data;
	int prev = MODMINUS(where,1,HISTORYSIZE);
	int i;

	fprintf(stdout,"%d %3.4f ",(int)data[where].time, data[where].value);

	for(i=0; i < PREDS; i++)
	{
		fprintf(stdout,"| %3.4f %3.4f %3.4f ",PRED(i,data[prev]),
			TPMSE(tp,i)/TPCOUNT(tp,i),
			TPMAE(tp,i)/TPCOUNT(tp,i));
	}
	fprintf(stdout,"| %d %d | ",MedWIN(6,data[prev]),MeanWIN(7,data[prev]));
	for(i=STATIC_PREDS; i < SECOND_ORDER_PREDS; i++)
	{
		fprintf(stdout,"%d ",TPPREDNUM(tp,i));
	}
	fprintf(stdout,"| ");
	for(i=SECOND_ORDER_PREDS; i < PREDS; i++)
	{
		fprintf(stdout,"%d ",TPPREDNUM(tp,i));
	}
	fprintf(stdout,"\n");
	fflush(stdout);

	return;
}


void
ThruputPrediction(tp)
struct thruput_pred *tp;
{
	 int tail = tp->tail;
	 int head = tp->head;
	 int i;
	 int prev;
	 double error;
	 double sq_error;
	 struct value_el *data = tp->data;
	 double value;
	 double run_mean = tp->run_mean;
	 double total = tp->total;
	 double tot_sum;
	 double min_run_p_error = MAXDOUBLE;
	 int min_run_p_i = 0;
	 double min_run_sq_error = MAXDOUBLE;
	 int min_run_sq_i = 0;
	 double prev_pred;
	 double absolute_error;
	 int last_error;
	 double min_win_sq_error = MAXDOUBLE;
	 double min_win_p_error = MAXDOUBLE;
	 int min_win_sq_i = 0;
	 int min_win_p_i = 0;
	 double min_tot_sq_error = MAXDOUBLE;
	 double min_tot_p_error = MAXDOUBLE;
	 int min_tot_sq_i = 0;
	 int min_tot_p_i = 0;
	 double win_error;
	 int error_window;
	 int datasize;

	 datasize = MODMINUS(head,tail,HISTORYSIZE) - 1;
	 if(ERR_WIN > datasize)
	 	error_window = datasize;
	 else
		error_window = ERR_WIN;

	 last_error = MODMINUS(head,error_window,HISTORYSIZE);



	value = data[head].value;
	prev = MODMINUS(head,1,HISTORYSIZE);
#ifdef GROT
	/*
	 * was the value within one deviation of the total prediction?
	 */
	avg_err = TPMAE(tp,TOT_MSE_PRED)/TPCOUNT(tp,TOT_MSE_PRED);
	if((value > (PRED(TOT_MSE_PRED,data[prev])-avg_err)) &&
	   (value < (PRED(TOT_MSE_PRED,data[prev])+avg_err)))
	{
		Onedev_mse++;
	}
	if((value > (PRED(TOT_MSE_PRED,data[prev])-2*avg_err)) &&
	   (value < (PRED(TOT_MSE_PRED,data[prev])+2*avg_err)))
	{
		Twodev_mse++;
	}

	avg_err = TPMAE(tp,TOT_MAE_PRED)/TPCOUNT(tp,TOT_MAE_PRED);
	if((value > (PRED(TOT_MAE_PRED,data[prev])-avg_err)) &&
	   (value < (PRED(TOT_MAE_PRED,data[prev])+avg_err)))
	{
		Onedev_mpe++;
	}
	if((value > (PRED(TOT_MAE_PRED,data[prev])-2*avg_err)) &&
	   (value < (PRED(TOT_MAE_PRED,data[prev])+2*avg_err)))
	{
		Twodev_mpe++;
	}

	ModalDev(Histogram,MedianPred,1.0,&hi,&lo);
	if((value > lo) && (value < hi))
		Onedev_modal++;
	ModalDev(Histogram,MedianPred,2.0,&hi,&lo);
	if((value > lo) && (value < hi))
		Twodev_modal++;

	Devcounts++;
#endif

	/*
	 * calculate the error our last prediction made
	 * now that we have the actual value
	 */
	for(i=0; i < PREDS; i++)
	{

        	error = data[head].value - PRED(i,data[prev]);
		if(TPCOUNT(tp,i) < MIN_DATA_HISTORY)
			error = 0.0;
        	sq_error = error * error;

        	SQERR(i,data[head]) = SQERR(i,data[prev])+sq_error;
		TPMSE(tp,i) += sq_error;
		if(error < 0.0)
			error = -1.0 * error;
		absolute_error = error;
        	ERR(i,data[head]) = ERR(i,data[prev])+absolute_error;
		TPMAE(tp,i) += absolute_error;
		TPCOUNT(tp,i) += 1.0;

	}

#ifdef GROT
	/*
	 * now calculate MSE values over different lifetimes
	 */
	if(datasize > LIFETIMES)
	{
		curr = prev;
		for(i=0; i < LIFETIMES; i++)
		{
			error = data[head].value - 
				PRED(TOT_MSE_PRED,data[curr]);
			Lifetime_mse[i] += (error * error); 
			error = data[head].value - 
				PRED(TOT_MAE_PRED,data[curr]);
			if(error < 0.0)
				error = -1.0 * error;
			Lifetime_mpe[i] += (error / data[head].value);
			curr = MODMINUS(curr,1,HISTORYSIZE);
		}
		Lifetime_count++;
	}
#endif





	/*
	 * use the current running mean
	 */
	NAME(0) = "Last value";
	PRED(0,data[head]) = value;


	/*
	 * use the current value as a prediction of the next value
	 */
	tot_sum = run_mean * total;
	tot_sum += data[head].value;
	total++;
	run_mean = tot_sum/total;
	tp->run_mean = run_mean;
	tp->total = total;
	NAME(1) = "Running Mean";
	PRED(1,data[head]) = run_mean;

	/*
	 * use the Van Jacobson method
	 */
	prev_pred = PRED(2,data[prev]);
	NAME(2) = "Van Jacobson";
	PRED(2,data[head]) = prev_pred + GAIN*(value - prev_pred);

	/*
	 * tell the median routines that the array is no longer valid
	 */
	Median_sorted = 0;
	NAME(3) = "Median";
	PRED(3,data[head]) = Median(tp,MED_N);
	NAME(4) = "Trimmed Median";
	PRED(4,data[head]) = TrimmedMedian(tp,MED_N,MED_ALPHA);

	/*
	 * Now use TrimmedMean to give a sliding window mean
	 */
	NAME(5) = "Sliding Window Mean";
	PRED(5,data[head]) = TrimmedMedian(tp,MED_N,0.0);
	NAME(6) = "Adjusted Median";
	AdjustedMedian(tp,6,WIN_MIN,WIN_MAX);
	NAME(7) = "Adjusted Mean";
	AdjustedMean(tp,7,WIN_MIN,WIN_MAX);

	for(i=0; i < STATIC_PREDS; i++)
	{
		if((TPMSE(tp,i)/TPCOUNT(tp,i)) < min_run_sq_error)
		{
			min_run_sq_error = TPMSE(tp,i)/TPCOUNT(tp,i);
			min_run_sq_i = i;
		}

		if((TPMAE(tp,i)/TPCOUNT(tp,i)) < min_run_p_error)
		{
			min_run_p_error = TPMAE(tp,i)/TPCOUNT(tp,i);
			min_run_p_i = i;
		}

		win_error = 
			(SQERR(i,data[head]) - SQERR(i,data[last_error])) /
				(double)error_window;
		if(win_error < min_win_sq_error)
		{
			min_win_sq_error = win_error;
			min_win_sq_i = i;
		}

		win_error = 
			(ERR(i,data[head]) - ERR(i,data[last_error])) /
				(double)error_window;
		if(win_error < min_win_p_error)
		{
			min_win_p_error = win_error;
			min_win_p_i = i;
		}

	}


	NAME(MSE_PRED) = "Min MSE Pred";
	PRED(MSE_PRED,data[head]) = PRED(min_run_sq_i,data[head]);
	TPPREDNUM(tp,MSE_PRED) = min_run_sq_i;
	NAME(MAE_PRED) = "Min MAE Pred";
	PRED(MAE_PRED,data[head]) = PRED(min_run_p_i,data[head]);
	TPPREDNUM(tp,MAE_PRED) = min_run_p_i;
	NAME(WIN_MSE_PRED) = "Win MSE Pred";
	PRED(WIN_MSE_PRED,data[head]) = PRED(min_win_sq_i,data[head]);
	TPPREDNUM(tp,WIN_MSE_PRED) = min_win_sq_i;
	NAME(WIN_MAE_PRED) = "Win MAE Pred";
	PRED(WIN_MAE_PRED,data[head]) = PRED(min_win_p_i,data[head]);
	TPPREDNUM(tp,WIN_MAE_PRED) = min_win_p_i;

	for(i=STATIC_PREDS; i < SECOND_ORDER_PREDS; i++)
	{
		if((TPMSE(tp,i)/TPCOUNT(tp,i)) < min_tot_sq_error)
		{
			min_tot_sq_error = TPMSE(tp,i)/TPCOUNT(tp,i);
			min_tot_sq_i = TPPREDNUM(tp,i);
		}
		if((TPMAE(tp,i)/TPCOUNT(tp,i)) < min_tot_p_error)
		{
			min_tot_p_error = TPMAE(tp,i)/TPCOUNT(tp,i);
			min_tot_p_i = TPPREDNUM(tp,i);
		}
	}
	NAME(TOT_MSE_PRED) = "Total MSE Pred";
	PRED(TOT_MSE_PRED,data[head]) = PRED(min_tot_sq_i,data[head]);
	TPPREDNUM(tp,TOT_MSE_PRED) = min_tot_sq_i;
	NAME(TOT_MAE_PRED) = "Total MAE Pred";
	PRED(TOT_MAE_PRED,data[head]) = PRED(min_tot_p_i,data[head]);
	TPPREDNUM(tp,TOT_MAE_PRED) = min_tot_p_i;

#ifdef PRINTPRED
	ModalDev(Histogram,MedianPred,2.0,&hi,&lo);
	fprintf(stdout,"%d %10.5f\n",data[head].time,
		PRED(TOT_MAE_PRED,data[prev]));
	
	fflush(stdout);
#endif

	return;

}

void
InitThruputPrediction(struct thruput_pred *tp)
{
	int i;

	tp->head = 0;
	tp->tail = 0;

/* ** NOT yet as we don't quite know what min and max values to use

	Histogram = DefineHistogram(MODES,
			DIM,
			Histomin,
			Histomax);
	MeanPred = DefineHistogramPred(Histogram);
	MedianPred = DefineHistogramPred(Histogram);
	MiddlePred = DefineHistogramPred(Histogram);
*/


        tp->mean = 0.0;
        tp->run_mean = 0.0;
        tp->total = 0.0;
	memset(tp->data,0,sizeof(tp->data));

	for(i=0; i < PREDS; i++)
	{
		TPMSE(tp,i) = 0.0;
		TPCOUNT(tp,i) = 0.0;
	}

	return;
}

/* not sure if exptime is right, I just wanted to avoid namespace collisions */
void
UpdateThruputPrediction(struct thruput_pred *tp,float value, time_t exptime)
{

	int head = tp->head;
	int tail = tp->tail;
	struct value_el *data = tp->data;
	int i;
        int data_size = MODMINUS(head,tail,HISTORYSIZE);

	tp->data[tp->head].time = exptime;
	tp->data[tp->head].value = value;

	/*
	 * a negative tail value says that we haven't filled the
	 * pipe yet.  See if this does it.
	 */
	if(data_size > MIN_DATA_HISTORY)
	{
		ThruputPrediction(tp);

	}
	else
	{
		for(i=0; i < PREDS; i++)
		{
			PRED(i,data[head]) = tp->data[head].value;
			TPMSE(tp,i) = 0.0;
			TPMAE(tp,i) = 0.0;
			ERR(i,data[head]) = 0.0;
			SQERR(i,data[head]) = 0.0;
			MedWIN(i,data[head]) = WIN_INIT;
			MeanWIN(i,data[head]) = WIN_INIT;
		}
		TPPREDNUM(tp,TOT_MSE_PRED) = 0;
		TPPREDNUM(tp,TOT_MAE_PRED) = 0;
	}



	tp->head = MODPLUS(head,1,HISTORYSIZE);

	/*
	 * if we have incremented head forward to that it is sitting on
	 * top of tail, then we need to bump tail.  The entire buffer is
	 * being used to fill the window.
	 */
	if(tp->head == tp->tail)
		tp->tail = MODPLUS(tp->tail,1,HISTORYSIZE);

	return;
}

#ifdef STANDALONE

#define PRED_ARGS "f:d:s:l:h:"

#include "format.h"

CalculateThruputPredictors(fname,tp)
char *fname;
struct thruput_pred *tp;
{
	FILE *fd;
	char line_buf[255];
	char *cp;
	time_t ltime;
	float value;
	int i;


	fd = fopen(fname,"r");

	if(fd == NULL)
	{
		fprintf(stderr,
			"CalculateThruputPredictors: unable to open file %s\n",
				fname);
		fflush(stderr);
		exit(1);
	}

	while(fgets(line_buf,255,fd))
	{
		cp = line_buf;

		while(isspace(*cp))
			cp++;

		for(i=1; i < TSFIELD; i++)
		{
			while(!isspace(*cp))
				cp++;

			while(isspace(*cp))
				cp++;
		}

		ltime = atoi(cp);

		cp = line_buf;

		for(i=1; i < VALFIELD; i++)
		{
			while(!isspace(*cp))
				cp++;

			while(isspace(*cp))
				cp++;
		}

		value = atof(cp);

		if(value != 0.0)
			UpdateThruputPrediction(tp,value,ltime);

	}

	fclose(fd);
}


struct thruput_pred Tp;

main(argc,argv)
int argc;
char *argv[];
{

	FILE *fd;
	int i;
	int j;
	double temp_mse;
	double temp_mpe;
	char fname[255];
	char c;


	if(argc < 2)
	{
		fprintf(stderr,"usage: testpred filename\n");
		fflush(stderr);
		exit(1);
	}

	fname[0] = 0;
	Histodim = DIM;
	Histostates = STATES;
	Histomax = HMAX;
	Histomin = HMIN;

	while((c = getopt(argc,argv,PRED_ARGS)) != EOF)
	{
		switch(c)
		{
			case 'f':
				SAFESTRCPY(fname,optarg);
				break;
			case 'd':
				Histodim = atoi(optarg);
				break;
			case 's':
				Histostates = atoi(optarg);
				break;
			case 'l':
				Histomin = atof(optarg);
				break;
			case 'h':
				Histomax = atof(optarg);
				break;
		}
	}

	if(fname[0] == 0)
	{
		fprintf(stderr,"usage: predictor -f fname [-d,s,l,h]\n");
		fflush(stderr);
		exit(1);
	}

	InitThruputPrediction(&Tp);
				
#if !defined(PRINTPRED)
	fprintf(stdout,"Calculating throughput predictors...");
	fflush(stdout);
#endif

	CalculateThruputPredictors(fname,&Tp);

	fprintf(stdout,"done.\n");
	fflush(stdout);

	for(i=0; i < PREDS; i++)
	{

		fprintf(stdout,"Predictor %d, MSE: %3.4f MAE: %3.4f, %s\n",
			i,TPMSE(&Tp,i)/TPCOUNT(&Tp,i),
				TPMAE(&Tp,i)/TPCOUNT(&Tp,i),
				NAME(i));
	}

	fprintf(stdout,"MSE within 1 dev: %3.2f, 2 dev: %3.2f\n",
			Onedev_mse/Devcounts,Twodev_mse/Devcounts);
	fprintf(stdout,"MAE within 1 dev: %3.2f, 2 dev: %3.2f\n",
			Onedev_mpe/Devcounts,Twodev_mpe/Devcounts);
	fprintf(stdout,"Modal within 1 dev: %3.2f, 2 dev: %3.2f\n",
			Onedev_modal/Devcounts,Twodev_modal/Devcounts);


#ifdef GROT
	fprintf(stdout,"\nlook ahead  MSE  MAE\n");
	for(i=0; i < LIFETIMES; i++)
	{
		temp_mse = 0.0;
		temp_mpe = 0.0;
		for(j=0; j <= i; j++)
		{
			temp_mse += (Lifetime_mse[j]/(double)Lifetime_count);
			temp_mpe += (Lifetime_mpe[j]/(double)Lifetime_count);
		}
		fprintf(stdout,"%d %f %f\n",i,
				temp_mse/(double)j,
				temp_mpe/(double)j);
	}
#endif

	fprintf(stdout,"Histogram\n");
	PrintHistogram(Histogram);
	fprintf(stdout,"\n");

	exit(0);

	
}
#endif
