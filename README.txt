Darin Baumgartel (dbaumgartel@gmail.com)
Developed in Sept, 2014
Based on W+Jets 7 TeV Blackhat+Sherpa analysis code developed by Joe Haley
For use on CERN lxplus6 clusters.
Please check the availability of the CMSSW version, as they change over time.

Checkout with:
git clone git@github.com:darinbaumgartel/BlackHatWJets8TeV.git BlackHatWJets8TeV


##########################################################################
### Part A: Transfer Blackhat ntuples from Castor to EOS               ###
##########################################################################

[1] Get list of files to transfer

    $ python LocateCastorFiles.py

    This writes out to FilesToTransfer.txt

[2] Stage all necessary files in castor

    $ python TransferBatcher.py --stage

    Allow some time (several hours or a day) for staging to complete

[3] First, run the TransferBatcher and check that the jobs
    created in the BatchJobs directory seem sensible.

    $ python TransferBatcher.py 
    $ cat BatchJobs/CP_0.tcsh

    You should seem a script filled with xrdcp commands (castor to EOS).

[4] Run the TransferBatcher to launch the jobs
    $ python TransferBatcher.py --submit

[5] Setup root (cmsenv) somewhere, and run the corruption tester. 
    This will attempt to open each root file, and if it fails, will
    mark the file as bad. At the end, it will print some cmsRm commands.
    If a few files are corrupt, issue these rm commands. 

    $ python CorruptionTest.py

[6] Repeat steps [4] and [5] until there are no more files to transfer, and
    there are no files noted as corrupt.



##########################################################################
### Part B: Unpack compiled utilities and build necessary packages     ###
##########################################################################

In the head directory there are tar.gz files containing:
1) A version of fastjet for jet clustering
2) A version of the ntuple reader 
   (i.e. https://blackhat.hepforge.org/trac/wiki/NtupleReaderInstallation)

These were compiled on lxplus, so we simply unpack them. 

    $ tar -zxvf fastjet-2.4.4-install_lxplus6.tar.gz
    $ tar -zxvf ntuplesreader-1.0-install_lxplus6.tar.gz


You need to be in an sh shell. First, setup tools.

    $ sh
    $ source /afs/cern.ch/cms/cmsset_default.sh
    $ cmsrel CMSSW_5_3_20
    $ cd CMSSW_5_3_20/src
    $ cmsenv
    $ cd -
    $ source ./setup.sh
    $ cd blackhat_hists
    $ make clean && make
    $ mkdir hists logs tmpjobs

The structure of the output histograms is defined in blackhat_hists/makeHistograms.cpp
If you change this file, rerun the make commands.

Note: Please inspect the makeHistograms.cpp file. If you want to change the 
      binning or the cuts, this is where you do it. If you make changes, simply
      rerun the make clean && make.

##########################################################################
### Part C: Get file sub-lists                                         ###    
##########################################################################

In blackhat there are different file types corresponding to different parts
of the NLO calculation. To run batch jobs, we must group like files into
small blocks, and then combine them later. From the blackhat_hists directory:

[1] Go to the lists subdirectory
    $ cd lists

[2] Make a log file of all the ntuples
    $ cmsLs -R /store/group/phys_smp/WPlusJets/BHSNtuplesV3 | grep ".root" > ntuplelog.txt

[3] Run the utility to split the ntuplelog into multiple lists
    $ python intelligent_splitter.py

[4] Check on the content of the split directory
    $ ls -l split



##########################################################################
### Part D: Launch jobs on the split lists of files                    ###    
##########################################################################

The JobLauncher.py script will take care of the running of the jobs. This 
script has a few important options.

    -n INT      : The number of jobs to launch (default 500)
                  e.g. -n 500
    -q queue    : The batch queue to use (default 1nd)
                  e.g. -q 8nh                 
    --scalevary : Also run variations in the factorization/renormalization
                  scale by a factor of 2.
    --pdfvary   : Also run every memeber of each of the PDF sets, including
                  CT10, MSTW, and NNPD. Warning, this is ~200 variations, and
                  takes a long time to run.                
    --clean     : Attempt to open every output file in the hists directory.
                  If corruption is detected, the file will be deleted. 
    --do        : Actually launch the batch jobs. Otherwise, the batch jobs
                  will only be created, and not launched. Run this first and
                  try a test job before launching in full. 

So, a normal workflow might look like this:

[1] Create some jobs, and do not launch
    $ python JobLauncher.py -n 100 -q 8nh
    Inspect jobs in the tmpjob directory, and try one bjob command to see if it works

[2] Launch the full run
    $ python JobLauncher.py  -n 100 -q 8nh --do
    Wait for jobs to finish.

[3] Clean root files
    $ python JobLauncher.py --clean

[4] Repeat steps 2 and 3 until all jobs are done

[5] Finish all these instructions, make plots, and see that you have sensible results

[6] When you are sure it has all worked correctly, come back to step 1, and repeat
    everything with --scalevary. PDF variations are very time consuming, and can be done
    by adding the --pdfvary flag. 






##########################################################################
### Part E: Merge the output of the jobs into single root files        ###    
##########################################################################

Now, the contents of the individual histograms can be merged. 

[1] From the EightTeVBlackhat directory, source the setup script 
    You should still be in an sh shell!
    $ source ./setup.sh

[2] From the blackhat_hists directory, run the histogram merger. This will
    do some manual merges using the combineHistograms.cpp script, and then
    some basic hadds on the output of that. If you do:
    $ python HistoGather.py 
    It will just print the commands to screen for inspection. 
    If you do:
    $ python HistoGather.py --do
    It will launch all the commands. 

The output will be in the hists directory, e.g.:
   hists/hists_CT10_r1.0_f1.0_m0/W1j_all.root
   hists/hists_CT10_r1.0_f1.0_m0/W2j_all.root
   hists/hists_CT10_r1.0_f1.0_m0/W3j_all.root

