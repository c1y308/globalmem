cmd_/home/a/globalfifo/Module.symvers := sed 's/\.ko$$/\.o/' /home/a/globalfifo/modules.order | scripts/mod/modpost -m -a  -o /home/a/globalfifo/Module.symvers -e -i Module.symvers   -T -
