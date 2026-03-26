from astropy.coordinates import SkyCoord, EarthLocation
from astropy.coordinates import solar_system
from astropy.time import Time
import astropy.units as u

# ---- Source coordinates (Westerhout 3) ----
src = SkyCoord(
    ra="02h27m04.1s",
    dec="+61d52m22s",
    frame="icrs",
    radial_velocity=0*u.km / u.s,
    distance=1*u.kpc,
    pm_ra_cosdec=0*u.mas/u.yr, 
    pm_dec=0*u.mas/u.yr
)

# ---- Observation time ----
t = Time.now()     # or specify Time("2026-03-11T00:00:00")

# ---- Observatory location (example: Biosphere 2) ----
loc = EarthLocation(lat=32.578778*u.deg,
                    lon=-110.850594*u.deg,
                    height=1164*u.m)

# ---- Compute barycentric velocity correction ----
barycorr = src.radial_velocity_correction(
    kind="barycentric",
    obstime=t,
    location=loc
)

print("Barycentric velocity correction:", barycorr.to(u.km/u.s))

# ---- Convert source to LSR frame to include solar motion ----
src_lsr = src.transform_to("lsr")

# Velocity component of solar motion along the LOS
v_sun_lsr = src_lsr.radial_velocity

print("Solar motion projected along LOS:", v_sun_lsr.to(u.km/u.s))

# ---- Total correction Earth + Sun relative to LSR ----
total_corr = barycorr + v_sun_lsr

print("Total velocity correction (Earth + Sun):", total_corr.to(u.km/u.s))
