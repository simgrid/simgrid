#!/usr/bin/env Rscript
args = commandArgs(trailingOnly=TRUE)
library(ggplot2)
df = read.csv(args[1], header=F, strip.white=T)
names(df) = c("Type", "Actor", "Container", "Start", "End", "Duration", "Level", "State"); 
ggplot(df) + 
    geom_segment(aes(x=Start, xend=End,
                     y=Actor, yend=Actor,color=State), size=5) +
    scale_fill_brewer(palette="Set1") +
    theme_bw() +
    theme (
        plot.margin = unit(c(0,0,0,0), "cm"),
        legend.spacing = unit(1, "mm"),
        panel.grid = element_blank(),
        legend.position = "top",
        legend.justification = "left",
        legend.box.spacing = unit(0, "pt"),
        legend.box.margin = margin(0,0,0,0)) -> p;

p.height <- length(unique(df$Actor)) * 0.05 + 2;
pdf(height = p.height)
plot(p)
dev.off()
