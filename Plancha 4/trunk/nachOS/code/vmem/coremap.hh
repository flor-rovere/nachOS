#ifndef COREMAP__HH
#define COREMAP__HH

#include "lib/bitmap.hh"
#include "userprog/address_space.hh"
#include "machine/machine.hh"
#include "filesys/open_file.hh"

class CoreMap : public BitMap{
public:
    CoreMap();
    ~CoreMap();

    int SelectVictim();
    int Find (AddressSpace *o, unsigned vpn);
    void Clear(unsigned which);

private:
    AddressSpace *owner[NUM_PHYS_PAGES];
    int vpns[NUM_PHYS_PAGES];
    int victim;
};

#endif
