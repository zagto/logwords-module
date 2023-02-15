KERNEL_SOURCE ?= /lib/modules/$(shell uname -r)/build

obj-m := logwords.o
logwords-y := printing.o proc.o module.o

all:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) clean
