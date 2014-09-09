Darin Baumgartel
Sept 4, 2014

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

