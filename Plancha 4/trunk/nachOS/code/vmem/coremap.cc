#include "coremap.hh"
#include "threads/system.hh"

CoreMap::CoreMap() 
{
    for (unsigned i = 0; i < NUM_PHYS_PAGES; i++)
    {
        owner[i] = NULL;
        vpns[i] = -1;
    }

    victim = 0;
}

CoreMap::~CoreMap()
{}

int
CoreMap::SelectVictim() 
{
    int vctm = victim;
    victim = (victim + 1) % NUM_PHYS_PAGES; 
    return vctm;
}

int     
CoreMap::Find(AddressSpace *o, unsigned vpn)
{
    int free = bitMap -> Find();
    unsigned vctm;
    if (free == -1)
    {
        vctm = SelectVictim();
        ASSERT(0 <= vctm && vctm < NUM_PHYS_PAGES);
        Thread *t = GetThread(vpns[vctm]);
        if (t)
        {
            DEBUG('c', "Llevando victima %d con vpn %d a swap\n", vctm, vpns[vctm]);
            t -> space -> SaveToSwap(vpns[vctm]);
            //owner[vctm] -> SaveToSwap(vpns[vctm]); TODO: dejar la linea de arriba o esta?
            free = vctm;
        }
    }
    else
        DEBUG('c', "Buscando paginas sin llevar a swap\n");

    owner[free] = o;
    vpns[free] = vpn;

    DEBUG('c', "Pagina elegida: %d\n", free);

    return free;
}

void
CoreMap::Clear(unsigned which)
{
    ASSERT(0 <= which && which < NUM_PHYS_PAGES);
    owner[which] = NULL;
    vpns[which] = -1;
}
