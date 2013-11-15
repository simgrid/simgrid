#include "network.hpp"

/***********
 * Classes *
 ***********/

class NetworkSmpiModel;
typedef NetworkSmpiModel *NetworkSmpiModelPtr;

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/

class NetworkSmpiModel : public NetworkCm02Model {
public:
  NetworkSmpiModel() : NetworkCm02Model() {m_haveGap=true;};
  void gapAppend(double size, const NetworkCm02LinkLmmPtr link, NetworkCm02ActionLmmPtr action);
  void gapRemove(ActionLmmPtr action);
  double latencyFactor(double size);
  double bandwidthFactor(double size);
  double bandwidthConstraint(double rate, double bound, double size);
  void communicateCallBack() {};
};


/************
 * Resource *
 ************/


/**********
 * Action *
 **********/



