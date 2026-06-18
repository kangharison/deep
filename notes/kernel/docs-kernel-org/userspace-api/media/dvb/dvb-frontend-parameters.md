# 6.1.1.3.frontend parameters

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dvb-frontend-parameters.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

type dvb\_frontend\_parameters

# 6.1.1.3. frontend parameters

The kind of parameters passed to the frontend device for tuning depend
on the kind of hardware you are using.

The struct `dvb_frontend_parameters` uses a `union with` specific
per-system parameters. However, as newer delivery systems required more
data, the structure size weren’t enough to fit, and just extending its
size would break the existing applications. So, those parameters were
replaced by the usage of
[FE\_GET\_PROPERTY/FE\_SET\_PROPERTY](fe-get-property.html#fe-get-property)
ioctl’s. The new API is flexible enough to add new parameters to
existing delivery systems, and to add newer delivery systems.

So, newer applications should use
[FE\_GET\_PROPERTY/FE\_SET\_PROPERTY](fe-get-property.html#fe-get-property)
instead, in order to be able to support the newer System Delivery like
DVB-S2, DVB-T2, DVB-C2, ISDB, etc.

All kinds of parameters are combined as a `union in` the
`dvb_frontend_parameters` structure:

```
struct dvb_frontend_parameters {
    uint32_t frequency;     /* (absolute) frequency in Hz for QAM/OFDM */
                /* intermediate frequency in kHz for QPSK */
    fe_spectral_inversion_t inversion;
    union {
        struct dvb_qpsk_parameters qpsk;
        struct dvb_qam_parameters  qam;
        struct dvb_ofdm_parameters ofdm;
        struct dvb_vsb_parameters  vsb;
    } u;
};
```

In the case of QPSK frontends the `frequency` field specifies the
intermediate frequency, i.e. the offset which is effectively added to
the local oscillator frequency (LOF) of the LNB. The intermediate
frequency has to be specified in units of kHz. For QAM and OFDM
frontends the `frequency` specifies the absolute frequency and is
given in Hz.

type dvb\_qpsk\_parameters

## 6.1.1.3.1. QPSK parameters

For satellite QPSK frontends you have to use the `dvb_qpsk_parameters`
structure:

```
struct dvb_qpsk_parameters {
    uint32_t        symbol_rate;  /* symbol rate in Symbols per second */
    fe_code_rate_t  fec_inner;    /* forward error correction (see above) */
};
```

type dvb\_qam\_parameters

## 6.1.1.3.2. QAM parameters

for cable QAM frontend you use the `dvb_qam_parameters` structure:

```
struct dvb_qam_parameters {
    uint32_t         symbol_rate; /* symbol rate in Symbols per second */
    fe_code_rate_t   fec_inner;   /* forward error correction (see above) */
    fe_modulation_t  modulation;  /* modulation type (see above) */
};
```

type dvb\_vsb\_parameters

## 6.1.1.3.3. VSB parameters

ATSC frontends are supported by the `dvb_vsb_parameters` structure:

```
struct dvb_vsb_parameters {
    fe_modulation_t modulation; /* modulation type (see above) */
};
```

type dvb\_ofdm\_parameters

## 6.1.1.3.4. OFDM parameters

DVB-T frontends are supported by the `dvb_ofdm_parameters` structure:

```
struct dvb_ofdm_parameters {
    fe_bandwidth_t      bandwidth;
    fe_code_rate_t      code_rate_HP;  /* high priority stream code rate */
    fe_code_rate_t      code_rate_LP;  /* low priority stream code rate */
    fe_modulation_t     constellation; /* modulation type (see above) */
    fe_transmit_mode_t  transmission_mode;
    fe_guard_interval_t guard_interval;
    fe_hierarchy_t      hierarchy_information;
};
```
