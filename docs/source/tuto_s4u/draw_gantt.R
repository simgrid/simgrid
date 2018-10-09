#!/usr/bin/env Rscript
args = commandArgs(trailingOnly=TRUE)
library(ggplot2)

# Load and relabel the data
df = read.csv(args[1], header=F, strip.white=T)
names(df) = c("Type", "Actor", "Container", "Start", "End", "Duration", "Level", "State");

# Actually draw the graph
p = ggplot(df) + geom_segment(aes(x=Start, xend=End, y=Actor, yend=Actor,color=State), size=5);

# Cosmetics to compact the resulting graph
p.height <- length(unique(df$Actor)) * 0.05 + 2;
pdf(height = p.height)

# Produce the pdf file
plot(p)
dev.off()
