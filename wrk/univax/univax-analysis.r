library(pacman)
p_load(data.table,ggplot2,magrittr)

age_groups = c('[0, 5)','[5, 18)','[18, 50)','[50, 65)','[65, 106)')

#d = fread('initial_experiment.csv')

mollinari.params = data.table(
  age = factor(age_groups, levels=age_groups),
  attack_rate.mean = c(0.203, 0.102, 0.066, 0.066, 0.090),
  attack_rate.stddev = c(0.062, 0.032, 0.017, 0.017, 0.024)
)

d = list()
for (n in c('seasonal_poe_output.csv')) {
  d.raw = fread(n)
  d.raw$age = factor(d.raw$age, levels=age_groups)
  d[[n]] = d.raw[,.(N_p=mean(N_p), IS_i=sum(IS_i), V_i=sum(V_i)),
          by=.(age,name,year=1+as.integer(day/360))]
}

ggplot(d$seasonal_poe_output.csv, aes(x=year, y=4+(100*IS_i/N_p))) +
  geom_hline(data=mollinari.params, color='blue',
             aes(yintercept=100*attack_rate.mean)) +
  geom_hline(data=mollinari.params, color='blue', linetype='dashed',
             aes(yintercept=100*(attack_rate.mean + attack_rate.stddev))) +
  geom_hline(data=mollinari.params, color='blue', linetype='dashed',
             aes(yintercept=100*(attack_rate.mean - attack_rate.stddev))) +
  #geom_line() +
  stat_summary(fun.data='mean_se', geom='ribbon',
               alpha=0.5, color='gray') +
  stat_summary(fun.y='mean', geom='line',
               linetype='dashed') +
  facet_wrap(~ age, ncol=1, scales='free') +
  ylab('Symptomatic Attack Rate\n(Percentage)') +
  xlab('Year') +
  theme(legend.position = 'none') +
  ggtitle('Yearly Symptomatic Attack Rate by Age Group for Baseline Vaccination')







local({
dt1 = data.table(age=c('[0, 5)','[5, 18)','[18, 50)','[50, 65)','[65, 106)'), cvg=c(0.70, 0.66, 0.32, 0.45, 0.65))
dt2 = d[year==1 & name=='70-30.report1.json_lines', .(age, N_p)]
dt3 = merge(dt1,dt2,by='age')
dt3[,cnt:=cvg*N_p]
tot = sum(dt3$cnt)
dt3[,prp:=cnt/tot]
print(dt3)
})
