host=akaros.cs.berkeley.edu
path=/var/www/parlib
groupid=akaros-web

install:
	rsync -rlvzC --delete-after ./* $(host):$(path)/
	-ssh $(host) chgrp -fR $(groupid) $(path)/\*
	-ssh $(host) chmod -fR 775 $(path)/\*
