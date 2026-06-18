# 6.1.1.1.Frontend type

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-type-t.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1.1.1. Frontend type

For historical reasons, frontend types are named by the type of
modulation used in transmission. The fontend types are given by
fe\_type\_t type, defined as:

type fe\_type

Frontend types

| fe\_type | Description | [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system) equivalent type |
| --- | --- | --- |
| `FE_QPSK` | For DVB-S standard | `SYS_DVBS` |
| `FE_QAM` | For DVB-C annex A standard | `SYS_DVBC_ANNEX_A` |
| `FE_OFDM` | For DVB-T standard | `SYS_DVBT` |
| `FE_ATSC` | For ATSC standard (terrestrial) or for DVB-C Annex B (cable) used in US. | `SYS_ATSC` (terrestrial) or `SYS_DVBC_ANNEX_B` (cable) |

Newer formats like DVB-S2, ISDB-T, ISDB-S and DVB-T2 are not described
at the above, as they’re supported via the new
[FE\_GET\_PROPERTY/FE\_GET\_SET\_PROPERTY](fe-get-property.html#fe-get-property)
ioctl’s, using the [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
parameter.

In the old days, struct [`dvb_frontend_info`](frontend-header.html#c.dvb_frontend_info "dvb_frontend_info")
used to contain `fe_type_t` field to indicate the delivery systems,
filled with either `FE_QPSK, FE_QAM, FE_OFDM` or `FE_ATSC`. While this
is still filled to keep backward compatibility, the usage of this field
is deprecated, as it can report just one delivery system, but some
devices support multiple delivery systems. Please use
[DTV\_ENUM\_DELSYS](fe_property_parameters.html#dtv-enum-delsys) instead.

On devices that support multiple delivery systems, struct
[`dvb_frontend_info`](frontend-header.html#c.dvb_frontend_info "dvb_frontend_info")::`fe_type_t` is
filled with the currently standard, as selected by the last call to
[FE\_SET\_PROPERTY](fe-get-property.html#fe-get-property) using the
[DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system) property.
