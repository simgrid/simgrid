# Copyright (c) 2003-2023. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.
 
import sys
import pandas as pd
import seaborn as sns 

df = pd.read_csv(sys.argv[1], names=['Time','Value', 'SoC - SoH'])
df['Time'] = df['Time'].apply(lambda x: float(x.split(" ")[-1]))
sns.set_theme()
sns.set(rc={'figure.figsize':(16,8)})
g = sns.lineplot(data=df, x='Time', y='Value', hue='SoC - SoH')
g.get_figure().savefig('battery_degradation.svg')