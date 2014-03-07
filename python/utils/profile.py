#!/usr/bin/env python
import pstats
p = pstats.Stats('x.log')
#p.strip_dirs().sort_stats(-1).print_stats()

#p.sort_stats('cumulative').print_stats(100)
p.sort_stats('time').print_stats(100)
