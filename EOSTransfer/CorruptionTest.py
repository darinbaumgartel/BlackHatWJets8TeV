import sys
from ROOT import *
import os

eosdir  = '/store/group/phys_smp/WPlusJets/BHSNtuplesV3'

print 'Reading list of files in ', eosdir
files = [afile.replace('\n','').split()[-1] for afile in os.popen('cmsLs -R '+eosdir+' | grep root').readlines()]
print '\n Starting corruption tests...\n' 
# files = ['/store/group/phys_smp/WPlusJets/BHSNtuplesV2/Wm5j/7TeV/V001/Wm5j_7TeV_V001_Et25.GeV_b34e89.root']
gEnv.GetValue("TFile.Recover", 0)
def CorruptionTest(afile):
	isGood = True
	f = TFile.Open('root://eoscms//eos/cms'+afile)
	print f
	isGood = '0x(nil)' not in str(f)
	if isGood==True:
		f.Close()
	else:
		print afile,'           ---> Corruption Detected'
	return [afile,isGood]
n = 0
CorrectedFileContent = []
for f in files:
	n+=1
	if n%100 == 0: 
		print n,'of',len(files),'tested.' 
	CorrectedFileContent.append(CorruptionTest(f))
print '\n\n'+'-'*100+'\n\n'

for c in CorrectedFileContent:
	if c[1]==False:
		print 'cmsRm ', c[0]
print '\n\n'+'-'*100+'\n\n'

