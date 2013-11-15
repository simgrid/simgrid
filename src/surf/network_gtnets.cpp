#include "network_gtnets.hpp"

static double time_to_next_flow_completion = -1;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_gtnets, surf,
                                "Logging specific to the SURF network GTNetS module");

extern routing_platf_t routing_platf;

double sg_gtnets_jitter = 0.0;
int sg_gtnets_jitter_seed = 10;

/*********
 * Model *
 *********/

void newRoute(int src_id, int dst_id,
		      xbt_dynar_t links, int nb_link)
{
  void *_link;
  NetworkGTNetsLinkPtr link;
  unsigned int cursor;
  int i = 0;
  int *gtnets_links;

  XBT_IN("(src_id=%d, dst_id=%d, links=%p, nb_link=%d)",
          src_id, dst_id, links, nb_link);

  /* Build the list of gtnets link IDs */
  gtnets_links = xbt_new0(int, nb_link);
  i = 0;
  xbt_dynar_foreach(links, cursor, _link) {
	link = (NetworkGTNetsLinkPtr) _link;
    gtnets_links[i++] = link->m_id;
  }

  if (gtnets_add_route(src_id, dst_id, gtnets_links, nb_link)) {
    xbt_die("Cannot create GTNetS route");
  }
  XBT_OUT();
}

void newRouteOnehop(int src_id, int dst_id,
                    NetworkGTNetsLinkPtr link)
{
  if (gtnets_add_onehop_route(src_id, dst_id, link->m_id)) {
    xbt_die("Cannot create GTNetS route");
  }
}

int NetworkGTNetsModel::addLink(ind id, double bandwidth, double latency)
{
  double bw = bandwidth * 8; //Bandwidth in bits (used in GTNETS).

  map<int,GTNETS_Link*>::iterator iter = p_links.find(id);
  xbt_assert((iter == p_links.end()), "Link %d already exists", id);

  if(iter == p_links.end()) {
    GTNETS_Link* link= new GTNETS_Link(id);
    p_links[id] = link;
  }

  XBT_DEBUG("Creating a new P2P, linkid %d, bandwidth %gl, latency %gl", id, bandwidth, latency);
  p_gtnetsLinks_[id] = new Linkp2p(bw, latency);
	  if(jitter_ > 0){
		XBT_DEBUG("Using jitter %f, and seed %u", jitter_, jitter_seed_);
		double min = -1*jitter_*latency;
		double max = jitter_*latency;
		uniform_jitter_generator_[id] = new Uniform(min,max);
		gtnets_links_[id]->Jitter((const Random &) *(uniform_jitter_generator_[id]));
	  }

	  return 0;
}

/************
 * Resource *
 ************/
NetworkGTNetsLink::NetworkGTNetsLink(NetworkGTNetsModelPtr model, const char* name, double bw, double lat, xbt_dict_t properties)
  :NetworkCm02Link(model, name, properties), m_bwCurrent(bw), m_latCurrent(lat)
{

  static int link_count = -1;

  if (xbt_lib_get_or_null(link_lib, name, SURF_LINK_LEVEL)) {
    return;
  }

  XBT_DEBUG("Scanning link name %s", name);

  link_count++;

  XBT_DEBUG("Adding new link, linkid %d, name %s, latency %g, bandwidth %g",
           link_count, name, lat, bw);



  if (gtnets_add_link(link_count, bw, lat)) {
    xbt_die("Cannot create GTNetS link");
  }
  m_id = link_count;

  xbt_lib_set(link_lib, name, SURF_LINK_LEVEL, this);
}

/**********
 * Action *
 **********/
