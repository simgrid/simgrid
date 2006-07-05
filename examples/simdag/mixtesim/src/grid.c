#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "simgrid.h"

#include "mixtesim_prototypes.h"

/**           **
 ** STRUCTURE **
 **           **/

/*
 * newHost()
 */
Host newHost()
{
  return (Host)calloc(1,sizeof(struct _Host));
}

/* 
 * freeHost()
 */
void freeHost(Host h)
{
  if (h->cpu_trace)
    free(h->cpu_trace);
  free(h);
  return;
}

/*
 * newLink()
 */
Link newLink()
{
  return (Link)calloc(1,sizeof(struct _Link));
}

/*
 * freeLink()
 */
void freeLink(Link l)
{
  if (l->latency_trace)
    free(l->latency_trace);
  if (l->bandwidth_trace)
    free(l->bandwidth_trace);
  free(l);
  return;
}

/*
 * newGrid()
 */
Grid newGrid()
{
  return (Grid)calloc(1,sizeof(struct _Grid));
}

/*
 * freeGrid()
 */
void freeGrid(Grid g)
{
  int i;
  for (i=0;i<g->nb_hosts;i++)
    freeHost(g->hosts[i]);
  if (g->hosts)
    free(g->hosts);
  for (i=0;i<g->nb_links;i++)
    freeLink(g->links[i]);
  if (g->links)
    free(g->links);
  if (g->connections) {
    for (i=0;i<g->nb_hosts;i++) {
      if (g->connections[i]) {
	free(g->connections[i]);
      }
    }
    free(g->connections);
  }
  free(g);
  return;
}


/**         **
 ** PARSING **
 **         **/

static int createConnectionMatrix(Grid grid);
static int parseLine(Grid,char*);
static int parseLinkCountLine(Grid,char*);
static int parseHostCountLine(Grid,char*);
static int parseLinkLine(Grid,char*);
static int parseHostLine(Grid,char*);

/*
 * parseGridFile()
 */
Grid parseGridFile(char *filename)
{
  Grid new;
  int i;

  new = newGrid();

  new->nb_host = SD_workstation_get_number();


  new->nb_link = SD_link_get_number();

  FILE *f;
  char line[4084];
  int count=0;
  char *tmp;

  Grid new;

  /* open Grid description file */
  if (!(f=fopen(filename,"r"))) {
    fprintf(stderr,"Impossible to open file '%s'\n",filename);
    return NULL;
  }

  /* Allocate memory for the Grid */
  new=newGrid();

  /* go through the lines in the files */
  while(fgets(line,4084,f)) {
    count++;
    if (line[0]=='#') /* comment line */
      continue;
    if (line[0]=='\n') /* empty line */
      continue;
    tmp=strdup(line); /* strdupoing seesm to make it work */
    if (parseLine(new,tmp) == -1) {
      fprintf(stderr,"Error in  Grid description file at line %d\n",count);
      fclose(f);
      return NULL;
    } 
    free(tmp);
  }
  fclose(f);

  /* Create the connection matrix */
  createConnectionMatrix(new);

  return new;
}

/*
 * parseLine()
 */
static int parseLine(Grid grid, char *line)
{
  char *ptr;
  /* does the line start with a keyword ? */
  ptr=strchr(line,' ');
  if (ptr==NULL) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  } else{
    *ptr='\0';
  }
  /* process the different key words */
  if (!strncmp(line,"LINK_COUNT",strlen("LINK_COUNT")))
    return parseLinkCountLine(grid,ptr+1);
  if (!strncmp(line,"LINK",strlen("LINK")))
    return parseLinkLine(grid,ptr+1);
  if (!strncmp(line,"HOST_COUNT",strlen("HOST_COUNT")))     
    return parseHostCountLine(grid,ptr+1);
  if (!strncmp(line,"HOST",strlen("HOST")))     
    return parseHostLine(grid,ptr+1);

  fprintf(stderr,"Unknown keyword %s\n",line);
  return -1;
}

/*
 * parseHostCountLine()
 *
 * HOST_COUNT <number>
 */
int parseHostCountLine(Grid grid, char *line)
{
  int i,count;

  /* HOST_COUNT should be called only once */
  if (grid->nb_hosts != 0) {
    fprintf(stderr,"Error: Only one Host Count specification allowed\n");
    return -1;
  }

  /* get the count */
  count=atoi(line);
  /* check that count >0 */
  if (count == 0) {
    fprintf(stderr,"Error: invalid Host count\n");
    return -1;
  }
  /* allocate the grid->hosts array */
  grid->nb_hosts=count;
  grid->hosts=(Host *)calloc(grid->nb_hosts,sizeof(Host));
  
  /* allocate the connection matrix */
  grid->connections=(Link ***)calloc(grid->nb_hosts,sizeof(Link**));
  for (i=0;i<grid->nb_hosts;i++) {
    grid->connections[i]=(Link **)calloc(grid->nb_hosts,sizeof(Link*));
  }
  return 0;
}

/*
 * parseLinkCountLine()
 *
 * LINK_COUNT <number>
 */
int parseLinkCountLine(Grid grid, char *line)
{
  int count;

  /* LINK_COUNT should be called only once */
  if (grid->nb_links != 0) {
    fprintf(stderr,"Error: Only one Link Count specification allowed\n");
    return -1;
  }

  /* get the count */
  count=atoi(line);
  if (count == 0) {
    fprintf(stderr,"Error: invalid Link count\n");
    return -1;
  }
  /* allocate the grid->links array */
  grid->nb_links=count;
  grid->links=(Link *)calloc(grid->nb_links,sizeof(Link));
  
  return 0;
}

/*
 * parseHostLine()
 *
 * HOST <index> <rel_speed> <cpu_file>
 */
int parseHostLine(Grid grid, char *line)
{
  char *ptr;
  Host host;
  char *s_indexlist,*s_cpu_file,*s_rel_speed,*s_mode;
  int *hostIndexList=NULL;
  int nb_hostOnLine, i, min,max;
  double rel_speed;

  /* HOST_COUNT must be called before HOST */
  if (grid->nb_hosts == 0) {
    fprintf(stderr,"Error: Host Count must be specified before Hosts\n");
    return -1;
  }

  /* Get index List*/
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_indexlist=line;
  *ptr='\0';
  line=ptr+1;

  /* Get rel_speed */
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_rel_speed=line;
  *ptr='\0';
  line=ptr+1;

  /* Get cpu_file */
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_cpu_file=line;
  *ptr='\0';
  line=ptr+1;

  /* get mode */
  ptr=strchr(line,'\n');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_mode=line;
  *ptr='\0';

  /* Process strings */
  ptr = strchr(s_indexlist,'-');
  min = atoi(s_indexlist);
  max = atoi(ptr+1);
  nb_hostOnLine = max-min+1;
  hostIndexList=(int*)calloc(max-min+1,sizeof(int));
  for (i=0;i<max-min+1;i++){
    hostIndexList[i]=min+i;
  }
  
  rel_speed=atof(s_rel_speed);

  /* Creating the hosts */
  for (i=0;i<nb_hostOnLine;i++){
    host = newHost();
    host->global_index = min+i;
    host->rel_speed = rel_speed;
    if (!host->global_index) {
      host->cluster=0;
    } else if (host->rel_speed != 
	       grid->hosts[host->global_index-1]->rel_speed) {
      host->cluster = grid->hosts[host->global_index-1]->cluster +1;
    } else {
      host->cluster = grid->hosts[host->global_index-1]->cluster;
    }

    host->cpu_trace = strdup(s_cpu_file);
    if (!strcmp(s_mode,"IN_ORDER")) {
      host->mode = SG_SEQUENTIAL_IN_ORDER;
    } else if (!strcmp(s_mode,"OUT_OF_ORDER")) {
      host->mode = SG_SEQUENTIAL_OUT_OF_ORDER;
    } else if (!strcmp(s_mode,"TIME_SLICED")) {
      host->mode = SG_TIME_SLICED;
    } else {
      fprintf(stderr,"Error: invalid mode specification '%s'\n",s_mode);
      return -1;
    }
    /* Is the host beyond the index */
    if (host->global_index >= grid->nb_hosts) {
      fprintf(stderr,"Error: More hosts than the host count\n");
      return -1;
    }
    
    /* Have we seen that host before ? */
    if (grid->hosts[host->global_index] != NULL) {
      fprintf(stderr,"Error: Two hosts share index %d\n",
	      host->global_index);
      return -1;
    }
    /* Add the host to the grid->hosts array */
    grid->hosts[host->global_index]=host;
  }
  return 1;
}

/*
 * parseLinkLine()
 *
 * LINK <index> <lat_file> <band_file>
 */
int parseLinkLine(Grid grid, char *line)
{
  char *ptr;
  Link link;
  char buffer[16];

  char *s_indexlist,*s_lat_file,*s_band_file,*s_mode;
  int *linkIndexList=NULL;
  int nb_linkOnLine, i, min,max;

  /* LINK_COUNT must be called before LINK */
  if (grid->nb_links == 0) {
    fprintf(stderr,"Error: Link Count must be specified before Links\n");
    return -1;
  }

  /* Get index */
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_indexlist=line;
  *ptr='\0';
  line=ptr+1;


  /* Get lat_file */
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_lat_file=line;
  *ptr='\0';
  line=ptr+1;

  /* Get band_file */
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_band_file=line;
  *ptr='\0';
  line=ptr+1;

  /* get mode */
  ptr=strchr(line,'\n');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_mode=line;
  *ptr='\0';

  /* Process strings */
  ptr = strchr(s_indexlist,'-');
  min = atoi(s_indexlist);
  if (ptr) {
    max = atoi(ptr+1);
  } else {
    max=min;
  }
  nb_linkOnLine = max-min+1;
  linkIndexList=(int*)calloc(max-min+1,sizeof(int));
  for (i=0;i<max-min+1;i++){
    linkIndexList[i]=min+i;
  }
  
  /* Creating the link */
  for (i=0;i<nb_linkOnLine;i++){
    link = newLink();
    link->global_index = min+i;
    link->latency_trace=strdup(s_lat_file);
    link->bandwidth_trace=strdup(s_band_file);
    if (!strcmp(s_mode,"IN_ORDER")) {
      link->mode = SG_SEQUENTIAL_IN_ORDER;
    } else if (!strcmp(s_mode,"OUT_OF_ORDER")) {
      link->mode = SG_SEQUENTIAL_OUT_OF_ORDER;
    } else if (!strcmp(s_mode,"TIME_SLICED")) {
      link->mode = SG_TIME_SLICED;
    } else if (!strcmp(s_mode,"FAT_PIPE")){
      link->mode = SG_FAT_PIPE;
    } else {
      fprintf(stderr,"Error: invalid mode specification '%s'\n",s_mode);
      return -1;
    }
    sprintf(buffer,"link%d",min+i);

    /* Is the node beyond the index */
    if (link->global_index >= grid->nb_links) {
    fprintf(stderr,"Error: More links than the link count\n");
    return -1;
    }
  

    /* Have we seen that link before ? */
    if (grid->links[link->global_index] != NULL) {
      fprintf(stderr,"Error: Two links share index %d\n",
	      link->global_index);
      return -1;
    }
    /* Add the link to the grid->links array */
    grid->links[link->global_index]=link;
  }
  return 1;
}

static int createConnectionMatrix(Grid grid){
  int i,j,nb_clusters;
  for (i=0;i<grid->nb_hosts;i++){
    for(j=0;j<grid->nb_hosts;j++){
      if (i==j) {
	grid->connections[i][j]=(Link*)calloc(1, sizeof(Link));
	grid->connections[i][j][0]=newLink();
	(grid->connections[i][j][0])->global_index=-1;
      }else{
	/* Intra cluster connection */
	if (grid->hosts[i]->cluster == grid->hosts[j]->cluster) {
	  grid->connections[i][j]=(Link*)calloc(3, sizeof(Link));
	  grid->connections[i][j][0]= grid->links[i];
	  grid->connections[i][j][1]= 
	    grid->links[grid->nb_hosts+grid->hosts[i]->cluster];
	  grid->connections[i][j][2]= grid->links[j]; 
	} else { /* Inter cluster connection */
	  grid->connections[i][j]=(Link*)calloc(7, sizeof(Link));

	  nb_clusters = grid->hosts[grid->nb_hosts-1]->cluster+1;

	  /* Src host */
	  grid->connections[i][j][0]= grid->links[i];
	  /* Src switch */
	  grid->connections[i][j][1]= 
	    grid->links[grid->nb_hosts+grid->hosts[i]->cluster];
	  /* Src gateway */
	  grid->connections[i][j][2]= 
	    grid->links[grid->nb_hosts+grid->hosts[i]->cluster+nb_clusters];
	  /* Backbone */
	  grid->connections[i][j][3]= 
	    grid->links[grid->nb_links-1];
	  /* Dest gateway */
	  grid->connections[i][j][4]= 
	    grid->links[grid->nb_hosts+grid->hosts[j]->cluster+nb_clusters];
	  /* Dest switch */
	  grid->connections[i][j][5]= 
	    grid->links[grid->nb_hosts+grid->hosts[j]->cluster];
	  /* Dest host */
	  grid->connections[i][j][6]= grid->links[j]; 
	
	}
      }
    }
  }
  return 1;
}

