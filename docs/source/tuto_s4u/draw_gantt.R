#!/usr/bin/env Rscript
args = commandArgs(trailingOnly=TRUE)
library(tidyverse)
library(pajengr)

# Load and relabel the data
df = pajeng_read(args[1])
df$state %>%
    # rename some columns to use simgrid terminology
    rename(Actor = Container,
           State = Value) %>%
    # do the plot
    ggplot() +
    geom_segment(aes(x=Start, xend=End, y=Actor, yend=Actor, color=State), size=5) -> p

# Cosmetics to compact the resulting graph
p.height <- length(unique(df$state$Actor)) * 0.05 + 2
pdf(height = p.height)

# Produce the pdf file
plot(p)
dev.off()
