cmd_/home/a/globalmem/Module.symvers := sed 's/\.ko$$/\.o/' /home/a/globalmem/modules.order | scripts/mod/modpost -m -a  -o /home/a/globalmem/Module.symvers -e -i Module.symvers   -T -
