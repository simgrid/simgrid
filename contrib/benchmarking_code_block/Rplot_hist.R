# R script showing .pdf file with plots of all injection histograms for a certain file
# Can be called from the command line with: Rscript $this_script.R inputfile

# Necessary libraries
library(plyr)
library(ggplot2)
library(data.table)
library(grid)

# Functions for arranging multiple plots
vp.layout <- function(x, y) viewport(layout.pos.row=x, layout.pos.col=y)
arrange_ggplot2 <- function(list, nrow=NULL, ncol=NULL, as.table=FALSE) {
  n <- length(list)
  if(is.null(nrow) & is.null(ncol)) { nrow = floor(n/2) ; ncol = ceiling(n/nrow)}
  if(is.null(nrow)) { nrow = ceiling(n/ncol)}
  if(is.null(ncol)) { ncol = ceiling(n/nrow)}
  ## NOTE see n2mfrow in grDevices for possible alternative
  grid.newpage()
  pushViewport(viewport(layout=grid.layout(nrow,ncol) ) )
  ii.p <- 1
  for(ii.row in seq(1, nrow)){
    ii.table.row <- ii.row
    if(as.table) {ii.table.row <- nrow - ii.table.row + 1}
    for(ii.col in seq(1, ncol)){
      ii.table <- ii.p
      if(ii.p > n) break
      print(list[[ii.table]], vp=vp.layout(ii.table.row, ii.col))
      ii.p <- ii.p + 1
    }
  }
}

### Main
# Reading command line argument with the input file path
args <- commandArgs(trailingOnly = TRUE)
fp  <- file(args[1], open = "r")

plots<-list()
i<-1

# Reading histograms one by one, line by line
while (length(oneLine <- readLines(fp, n = 1, warn = FALSE)) > 0){
  myVector <- (strsplit(oneLine, "\t")) 
  
  dfl <- ldply (myVector, data.frame)

  name<-as.character(dfl[1,])
  nbins<-as.numeric(as.character(dfl[4,]))
  allbreaks<-as.numeric(as.character(dfl[5:(5+nbins-1),]))
  
  dh<-data.frame(Name=as.character(dfl[1,]), Total=as.numeric(as.character(dfl[2,])),
                 Mean=as.numeric(as.character(dfl[3,])), Nbins=as.numeric(as.character(dfl[4,])))
  dh<-cbind(dh,Bstart=allbreaks[-length(allbreaks)])
  dh<-cbind(dh,Bend=allbreaks[-1])
  dh<-cbind(dh,Density=as.numeric(as.character(dfl[(5+nbins):(5+nbins*2-2),])))

  # Plotting single histogram, if it only has one value then use geom_bar
  if (nbins > 2)
    plots[[i]]<-ggplot(data=data.frame(dh), aes(xmin=Bstart, xmax=Bend, ymin=0, ymax=Density)) +
        geom_rect(aes(fill=Density)) + theme_bw() + scale_x_continuous("Time [s]", allbreaks) +
            labs(title=name, y=element_text("Density %"))
  else
    plots[[i]]<-ggplot(data=data.frame(dh), aes(factor(Bstart))) + geom_bar(aes(fill=Density)) +
        theme_bw() + labs(title=name, y=element_text("Density %"), x=element_text("Time [s]"))
  i<-i+1
}

# Printing all plots together in a table
arrange_ggplot2(plots, as.table=TRUE)

write("Done producing a histogram plot. Open Rplots.pdf located in this folder to see the results", stdout())
