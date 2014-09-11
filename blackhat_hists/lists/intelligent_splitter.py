import os

log = [(x.replace('\n','')).split() for x in os.popen('cat ntuplelog.txt | grep root').readlines()]

fileinfo = []
suminfo = []
for ll in log:
	fileinfo.append([float(ll[1]), ll[-1].split('Jets/')[-1]])

# uniqs = []

sumf = 0.0
for f in fileinfo:
	sumf += f[0]

targetf = (1.0*sumf)/2130.

PTYPES = []
CTYPES = []

for f in fileinfo:
	ptype = f[1].split('/')[1]
	ctype = f[1].split('/')[3]
	if ptype not in PTYPES:
		PTYPES.append(ptype)
	if ctype not in CTYPES:
		CTYPES.append(ctype)

PTYPES.sort()
CTYPES.sort()
print PTYPES
print CTYPES

FULLTYPES = []

for _f in fileinfo:
	f = _f[-1]
	for c in CTYPES:
		if c not in f:
			continue
		for p in PTYPES:
			if p not in f:
				continue
			info = [c,p,c+'_'+p+'.list_']
			if info not in FULLTYPES:
				FULLTYPES.append(info)

# FULLTYPES.sort()

os.system('rm -r split/*')

def makelist(fulltype,_n,filelist):
	nmarker = str(_n)
	if len(nmarker)==1:
		nmarker = '0'+nmarker
	name = 'split/'+fulltype[-1]+nmarker
	afile = open(name,'w')
	for tfile in filelist:
		afile.write(tfile+'\n')
	afile.close()




for F in FULLTYPES:
	print F
	runningtotal = 0.0
	filelist = []
	findex = 0
	for _f in fileinfo:
		[t,f] = _f
		if F[0] not in f or F[1] not in f:
			continue
		runningtotal += t
		filelist.append(f)
		if (t > targetf) or len(filelist)>20:
			findex += 1
			makelist(F,findex,filelist)
			filelist = []
			runningtotal = 0.0

	if len(filelist)>0:
		findex += 1
		makelist(F,findex,filelist)
		filelist = []
		runningtotal = 0.0



