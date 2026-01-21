cmd_/home/a/globalmem/modules.order := {   echo /home/a/globalmem/globalmem.ko; :; } | awk '!x[$$0]++' - > /home/a/globalmem/modules.order
