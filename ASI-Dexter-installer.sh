#!/bin/bash
# ASI Medipix3RX Dexter 
# NB 2016-11-05

clear
echo ""
echo "Starting ASI Medipix3RX Dexter installation script at "$(date)
echo ""

if [ ${#@} -lt 1 ]
then
    echo "use: $0 FILENAME.tar.gz"
    exit 1
fi

PAYLOAD=$1

if [ -f $PAYLOAD ]
then
    echo "Found file: $PAYLOAD"
else
    echo "Count not find file: $PAYLOAD"
    exit 1
fi

echo $1


#===================================================================================
# If installation folder already exists, 
#    backup old installation folder 
#    then make a new directory and continue

DESTINATION="ASI-Dexter"

if [ -d $HOME/$DESTINATION ]
then
    DATE=`date --rfc-3339=ns | tr " " _`
    echo "Backing up old installation foler to: "$HOME/$DESTINATION"_backup_"$DATE
    mv $HOME/$DESTINATION $HOME/$DESTINATION"_backup_"$DATE
fi

echo "Making installation folder: " $HOME/$DESTINATION
mkdir -p $HOME/$DESTINATION

#===================================================================================
# If it exists, remove temporary directory
#    Then make a new one
# Extract tar to that temporary folder

TMPDIR=$HOME"/.ASI-Dexter-installer"

#PAYLOAD="$HOME/Downloads/ASI-Dexter.tar.gz"
if [ ! -f $PAYLOAD ]
then
    echo "Installation file ${PAYLOAD} does not exist"
    exit 1
else
    echo "Installation file found at ${PAYLOAD}"
fi

if [ -d $TMPDIR ]
then
    echo "Removing existing temporary directory"
    rm -rf $TMPDIR
fi

echo "Making temporary installation folder: " $TMPDIR
echo ""
mkdir -p $TMPDIR

echo "Extracting: "$PAYLOAD
echo "to folder: "$TMPDIR
echo ""

tar -xf $PAYLOAD -C $TMPDIR

# Basic file existance check
if [ ! -f "${TMPDIR}/Mpx3GUI" ]
then
    echo "Extraction failed - Mpx3GUI file not found in temporary directory"
    exit 1
else
    echo "Extraction probably successful."
fi

#===================================================================================

# Actually install the program folder
cp -r ${TMPDIR}/* ${HOME}/${DESTINATION}/

#===================================================================================

echo ""
echo "Installation complete. Enjoy :) "
echo $(date)
echo ""

chmod +x ${HOME}/${DESTINATION}"/Mpx3GUI"
cd ${HOME}/${DESTINATION}
./ASI-Medipix3RX.sh

# Exit with success
exit 0
