Here are the components needed for supporting the PPPoATM protocol under
linux kernel 2.4.0-test2. You should be familiar with 'patch' and
building linux kernels and tools from sourcecode. This is *not* ready
for novices.


      Kernel patch

This is versus 2.4.0-test2:

    * pppoatm-kernel-vs-2.4.0test2.diff.gz 

The PPPoATM code has only been tested as a module, but it hopefully also
works compiled-in to the kernel.

*NOTE:* Jens Axboe has been making minor modicifactions to the above
patch to make it work on more recent 2.4 kernels. You're probably better
off getting a version from kernel.org directory
<http://www.kernel.org/pub/linux/kernel/people/axboe/PPPoATM/> instead.


      pppd patch

    * Start with ppp-2.4.0b2:
      ppp-2.4.0b2.tar.gz
    * Add in the PPPoE patch maintained by Michal Ostrowski. This patch
      includes work done by myself and others to make pppd gracefully
      handle transports other than serial lines using plugins. In fact,
      this patch includes an early non-fuctional version of the PPPoATM
      code.
      pppd.patch.240600
    * Finally, add in my pppd patch. This fixes the PPPoATM support in
      the above patch.
      pppoatm-pppd-vs-2.4.0b2+240600.diff.gz 

Note that to sucessfully compile pppd with the PPPoATM patch you need a
recent version of the linux-atm <http://linux-atm.sourceforge.net/>
userland environment installed (in order to have the correct libraries
and header files).

NEW: I'm not actively doing PPPoATM work anymore. However, Dennis Monks
has contributed this replacement pppoatm.c <pppoatm.c-for-2.4.2b3> that
supposedly allows pppoatm to work with pppd 2.4.2b3. From his email:

> I am not sure if are still working on this, but I have modified your
> source so it works with 2.4.2b3. Alot of the code was derived from
> rp-pppoe. I still have one bug with setdevname_pppoatm when it is
> called by process_options, a false seems to be returned, even though
> it is true. 


      Example of pppd use

(From the pppd subdirectory of the ppp-2.4.0b2 install)

./pppd plugin plugins/pppoatm.so 0.80 192.0.2.1:192.0.2.2

Here 0.80 is the /vpi/./vci/ to use, 192.0.2.1 is the local IP address,
and 192.0.2.2 is the remote IP address.


      Good luck!

------------------------------------------------------------------------
mitch@sfgoth.com

