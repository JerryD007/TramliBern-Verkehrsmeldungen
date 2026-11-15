#pragma once
#include "Model.h"
#include "astronomy.h"
// Astronomy Engine 2.1.19, UTC days since J2000 (2000-01-01 12:00 UTC).
inline Moon calculateMoon(time_t now){
 Moon m;if(!timeValid(now))return m;
 auto t=Astronomy_TimeFromDays((double(now)-946728000.0)/86400.0);
 auto p=Astronomy_MoonPhase(t);auto light=Astronomy_Illumination(BODY_MOON,t);
 auto full=Astronomy_SearchMoonPhase(180,t,32);
 if(p.status!=ASTRO_SUCCESS||light.status!=ASTRO_SUCCESS||full.status!=ASTRO_SUCCESS)return m;
 m.phase=p.angle/360.0;m.lit=light.phase_fraction*100.0;m.full=llround(full.time.ut*86400.0+946728000.0);m.valid=true;return m;
}
