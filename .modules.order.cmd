cmd_/home/a/globalfifo/modules.order := {   echo /home/a/globalfifo/globalfifo.ko; :; } | awk '!x[$$0]++' - > /home/a/globalfifo/modules.order
