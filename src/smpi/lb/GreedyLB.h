#ifndef _GREEDYLB_H_
#define _GREEDYLB_H_


//void CreateGreedyLB();
//BaseLB * AllocateGreedyLB();


#define CkPrintf printf

#include "ckgraph.h"


//TODO We need class ProcInfo
//TODO Find out where is Vertex declared

//class GreedyLB : public CBase_GreedyLB {
class GreedyLB{

public:
  struct HeapData {
    double load;
    int    pe;
    int    id;
  };

  struct ProcStats {		// per processor data
    int n_objs;			// number of objects on the processor
    double pe_speed;		// processor frequency
//#if defined(TEMP_LDB)
//	float pe_temp;
//#endif
//
//    /// total time (total_walltime) = idletime + overhead (bg_walltime)
//    ///                             + object load (obj_walltime)
//    /// walltime and cputime may be different on shared compute nodes
//    /// it is advisable to use walltime in most cases
    double total_walltime;
//    /// time for which the processor is sitting idle
    double idletime;
//    /// bg_walltime called background load (overhead in ckgraph.h) is a
//    /// derived quantity: total_walltime - idletime - object load (obj_walltime)
    double bg_walltime;
//#if CMK_LB_CPUTIMER
//    double total_cputime;
//    double bg_cputime;
//#endif
//    // double utilization;
    int pe;			// processor id
    bool available;
    ProcStats(): n_objs(0), pe_speed(1), total_walltime(0.0), idletime(0.0),
//#if CMK_LB_CPUTIMER
//		 total_cputime(0.0), bg_cputime(0.0),
//#endif
	   	 bg_walltime(0.0), pe(-1), available(true) {}
//    inline void clearBgLoad() {
//      idletime = bg_walltime = 
//#if CMK_LB_CPUTIMER
//      bg_cputime = 
//#endif
//      0.0;
//    }
//    inline void pup(PUP::er &p) {
//      p|total_walltime;
//      p|idletime;
//      p|bg_walltime;
//#if CMK_LB_CPUTIMER
//      p|total_cputime;
//      p|bg_cputime;
//#endif
//      p|pe_speed;
//      if (_lb_args.lbversion() < 1 && p.isUnpacking()) {
//         double dummy;  p|dummy;    // for old format with utilization
//      }
//      p|available; p|n_objs;
//      if (_lb_args.lbversion()>=2) p|pe; 
//    }
  };


  struct LDStats {
    int count;			// number of processors in the array "procs"
    ProcStats *procs;		// processor statistics
//
    int n_objs;			// total number of objects in the vector "objData"
//    int n_migrateobjs;		// total number of migratable objects
    CkVec<LDObjData> objData;	// LDObjData and LDCommData defined in lbdb.h
    CkVec<int> from_proc;	// current pe an object is on
    CkVec<int> to_proc;		// new pe you want the object to be on
//
//    int n_comm;			// number of edges in the vector "commData"
//    CkVec<LDCommData> commData;	// communication data - edge list representation
//				// of the communication between objects
//
//    int *objHash;		// this a map from the hash for the 4 integer
//				// LDObjId to the index in the vector "objData"
//    int  hashSize;
//
//    int complete_flag;		// if this ocg is complete, eg in HybridLB,
//    // this LDStats may not be complete
//
//    int is_prev_lb_refine;
//    double after_lb_max;
//    double after_lb_avg;
//
//    LDStats(int c=0, int complete_flag=1);
//    /// the functions below should be used to obtain the number of processors
//    /// instead of accessing count directly
    inline int nprocs() const { return count; }
//    inline int &nprocs() { return count; }
//
//    void assign(int oid, int pe) { CmiAssert(procs[pe].available); to_proc[oid] = pe; }
//    /// build hash table
//    void makeCommHash();
//    void deleteCommHash();
//    /// given an LDObjKey, returns the index in the objData vector
//    /// this index changes every time one does load balancing even within a run
//    int getHash(const LDObjKey &);
//    int getHash(const LDObjid &oid, const LDOMid &mid);
//    int getSendHash(LDCommData &cData);
//    int getRecvHash(LDCommData &cData);
//    void clearCommHash();
//    void clear() {
//      n_objs = n_migrateobjs = n_comm = 0;
//      objData.free();
//      commData.free();
//      from_proc.free();
//      to_proc.free();
//      deleteCommHash();
//    }
//    void clearBgLoad() {
//      for (int i=0; i<nprocs(); i++) procs[i].clearBgLoad();
//    }
//    void computeNonlocalComm(int &nmsgs, int &nbytes);
//    double computeAverageLoad();
//    void normalize_speed();
//    void print();
//    // edit functions
//    void removeObject(int obj);
//    void pup(PUP::er &p);
//    int useMem();
  };
  //GreedyLB(const CkLBOptions &);
  GreedyLB();
  //GreedyLB(CkMigrateMessage *m):CBase_GreedyLB(m) { lbname = "GreedyLB"; }
  void work(LDStats* stats);
private:
	enum           HeapCmp {GT = '>', LT = '<'};
    	void           Heapify(HeapData*, int, int, HeapCmp);
	void           HeapSort(HeapData*, int, HeapCmp);
	void           BuildHeap(HeapData*, int, HeapCmp);
	bool        Compare(double, double, HeapCmp);
	HeapData*      BuildCpuArray(BaseLB::LDStats*, int, int *);  
	HeapData*      BuildObjectArray(BaseLB::LDStats*, int, int *);      
	bool        QueryBalanceNow(int step);
};

#endif 

