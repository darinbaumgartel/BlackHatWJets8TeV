
INCS =  -I/afs/cern.ch/work/d/darinb/BlackHat/CMSSW_5_3_8_Blackhat/src/WJetsAnalysis/BlackHatHistogramMaker/ntuplesreader-1.0-install/include/ntuplesreader -pthread -m64 -I/afs/cern.ch/cms/slc5_amd64_gcc462/cms/cmssw/CMSSW_5_3_8/external/slc5_amd64_gcc462/bin/../../../../../../lcg/root/5.32.00-cms21/include\
	-I/afs/cern.ch/work/d/darinb/BlackHat/CMSSW_5_3_8_Blackhat/src/WJetsAnalysis/BlackHatHistogramMaker/fastjet-2.4.4-install/include
LIBS =  -L/afs/cern.ch/work/d/darinb/BlackHat/CMSSW_5_3_8_Blackhat/src/WJetsAnalysis/BlackHatHistogramMaker/ntuplesreader-1.0-install/lib/ -lNTR -L/afs/cern.ch/cms/slc5_amd64_gcc462/cms/cmssw/CMSSW_5_3_8/external/slc5_amd64_gcc462/bin/../../../../../../lcg/root/5.32.00-cms21/lib -lCore -lCint -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -pthread -lm -ldl -rdynamic\
	-Wl,-rpath,/afs/cern.ch/work/d/darinb/BlackHat/CMSSW_5_3_8_Blackhat/src/WJetsAnalysis/BlackHatHistogramMaker/fastjet-2.4.4-install/lib -lm -L/afs/cern.ch/work/d/darinb/BlackHat/CMSSW_5_3_8_Blackhat/src/WJetsAnalysis/BlackHatHistogramMaker/fastjet-2.4.4-install/lib -lfastjet


HDRS =  BH_hist.h Particle.h
EXES = makeHistograms.exe combineHistograms.exe

all: $(EXES)
	@echo "Done."

BH_hist.o: BH_hist.h BH_hist.cpp
	g++ -c $(INCS) -o BH_hist.o BH_hist.cpp

Particle.o: Particle.h Particle.cpp
	g++ -c $(INCS) -o Particle.o Particle.cpp

makeHistograms.o: makeHistograms.cpp BH_hist.cpp
	g++ -c $(INCS) -o makeHistograms.o makeHistograms.cpp

makeHistograms.exe: makeHistograms.o  BH_hist.o Particle.o
	@rm -f makeHistograms.exe
	g++ $(LIBS) -o makeHistograms.exe makeHistograms.o  BH_hist.o Particle.o


combineHistograms.o: combineHistograms.cpp
	g++ -c $(INCS) -o combineHistograms.o combineHistograms.cpp

combineHistograms.exe: combineHistograms.o
	@rm -f combineHistograms.exe
	g++ $(LIBS) -o combineHistograms.exe combineHistograms.o

clean :
	rm -f *.o *.exe
