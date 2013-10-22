# R script that produces histograms from benchmarked values

# Can be called from the bash script with the following code:
# export R_INPUT=$inputfile
# export R_OUTPUT=$outputfile
# R CMD BATCH $this_script.R

# Use functions from bench.h to benchmark execution time of the desired block,
# then Rhist.R script to read all timings and produce histograms
# and finally inject.h to inject values instead of executing block

  
inputfile<-Sys.getenv("R_INPUT")
outputfile<-Sys.getenv("R_OUTPUT")

df<-read.table(inputfile,header=F)
df<-df[,c(1,4)]
names(df)<-c("NAME","TIME")
attach(df)

for(i in unique(NAME))
{
  vector1<-df[NAME==i,2]
  h<-hist(vector1)
  
  cat(i, file = outputfile, sep = "\t", append = TRUE)
  cat("  ", file = outputfile, sep = "\t", append = TRUE)
  cat(sprintf("%.8f", mean(vector1)), file =outputfile, sep = "\t ", append = TRUE)
  cat("\t", file = outputfile,  append = TRUE)
  cat(length(h$breaks), file = outputfile, append = TRUE)
  cat("\t", file = outputfile,  append = TRUE)
  cat(sprintf("%.8f", h$breaks), file = outputfile, sep = " \t", append = TRUE)
  cat("\t", file = outputfile,  append = TRUE)
  h$density = h$counts/sum(h$counts)
  cat(sprintf("%.14f", h$density), file = outputfile, sep = " \t", append = TRUE)
  cat("\n", file = outputfile,  append = TRUE)
}
