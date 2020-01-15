# file:		update.sh
# contents:	shell script to update sources from CNMAT homepage
# author:	stefan kersten <steve@k-hornz.de>
# comments:	requires wget
# usage:	sh update.sh

base_url="http://cnmat.cnmat.berkeley.edu/OpenSoundControl/src/"
wget -r -np -nH --cut-dirs=2 -R .html $base_url

# EOF
