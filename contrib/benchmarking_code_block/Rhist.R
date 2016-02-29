# R script that produces histograms from benchmarked values
# Can be called from the bash script with the following code:
# export R_INPUT=$inputfile
# export R_OUTPUT=$outputfile
# export R_TYPE=$hist_type

# R CMD BATCH $this_script.R or  Rscript $this_script.R

# Use functions from bench.h to benchmark execution time of the desired block, then Rhist.R script to read all timings
# and produce histograms and finally inject.h to inject values instead of executing block

# This is a small function to help merging empty nbins for dhist histograms
merge_empty_bins <- function (h){
  i<-1
  j<-1
  counts2<--1
  breaks2<-h$breaks[1]

  if (length(h$counts)>1)
    for(i in 1:(length(h$counts)-1)){
      if(h$counts[i]!=0 || h$counts[i+1]!=0){
        counts2[j]<-h$counts[i]
        breaks2[j+1]<-h$breaks[i+1];
        j<-j+1
      }
    }
  
  counts2[j]<-h$counts[length(h$counts)]
  breaks2[j+1]<-h$breaks[length(h$breaks)]

  h$counts<-counts2
  h$breaks<-breaks2

  return (h)
}

# Main
source("analysis/hist_script/Rdhist.R")

inputfile<-Sys.getenv("R_INPUT")
outputfile<-Sys.getenv("R_OUTPUT")
type<-Sys.getenv("R_TYPE")

if (!(type %in% c("mean","default","sturges","scott"))){stop("Wrong histogram type")}

df<-read.table(inputfile,header=F)
df<-df[,c(1,4)]
names(df)<-c("NAME","TIME")
attach(df)

for(i in unique(NAME)){
  vector1<-df[NAME==i,2]
  if (length(vector1)==1){
    #If there is only one element
    h<-hist(vector1) # Just for R compatibility reasons
    h$breaks<-c(vector1,vector1)
    h$counts<-1
  } else {
    if (type=="mean"){
      #Mean value only
      h<-hist(vector1) # Just for R compatibility reasons
      h$breaks<-c(mean(vector1),mean(vector1))
      h$counts<-length(vector1)
    } else
       if (type=="default")
         #Standard HISTOGRAM:
         h<-hist(vector1)
       else {
         #Dhist:
         h<-dhist(vector1,nbins=type, plot = FALSE, lab.spikes = FALSE, a=5*iqr(vector1), eps=0.15)
         h$breaks<-h$xbr
         h$count<-as.vector(h$counts)
         h$counts<-h$count
         h<-merge_empty_bins(h)
       }
  }
    
  cat(i, file = outputfile, sep = "\t", append = TRUE)
  cat("\t", file = outputfile,  append = TRUE)
  cat(sum(h$counts), file =outputfile, sep = "\t", append = TRUE)
  cat("\t", file = outputfile,  append = TRUE)
  cat(sprintf("%.8f", mean(vector1)), file =outputfile, sep = "\t", append = TRUE)
  cat("\t", file = outputfile,  append = TRUE)
  cat(length(h$breaks), file = outputfile, append = TRUE)
  cat("\t", file = outputfile,  append = TRUE)
  cat(sprintf("%.8f", h$breaks), file = outputfile, sep = "\t", append = TRUE)
  cat("\t", file = outputfile,  append = TRUE)
  h$density = h$counts/sum(h$counts)
  cat(sprintf("%.8f", h$density), file = outputfile, sep = "\t", append = TRUE)
  cat("\n", file = outputfile,  append = TRUE)
}
