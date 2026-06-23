# Fake CAL
Y=2.0
As=400
lvl=8000
sig=15
for i in `seq 0 1 1023`;do echo $i| \
	awk -v Y=$Y -v lvl=$lvl -v As=$As 'BEGIN{srand();} \
	{printf("%d\n", Y*(lvl+As*sin($1*3*2*3.14/360)+10*rand()))}';done > fake_cal.txt
# Fake REF
for i in `seq 0 1 1023`;do echo $i| \
	awk -v lvl=$lvl -v As=$As 'BEGIN{srand();} \
	{printf("%d\n", lvl+As*sin($1*3*2*3.14/360)+10*rand())}';done > fake_ref.txt
# Fake SIG
for k in `seq 0 1 4`;do \
    for i in `seq 0 1 1023`;do echo $i| \
	awk -v A=`echo 30+30*$k|bc` -v mu=`echo 450+2*$k|bc` -v lvl=$lvl -v As=$As \
	'BEGIN{srand();} \
	{printf("%d\n", lvl+As*sin($1*3*2*3.14/360)+10*rand()+ \
	A*exp(-(($1-mu)^2/(2*15^2))))}';done > fake_sig$k.txt ;done
