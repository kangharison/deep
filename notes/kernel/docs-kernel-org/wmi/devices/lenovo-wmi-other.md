# 5.Lenovo WMI Interface Other Mode Driver (lenovo-wmi-other)

> 출처(원문): https://docs.kernel.org/wmi/devices/lenovo-wmi-other.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5. Lenovo WMI Interface Other Mode Driver (lenovo-wmi-other)

## 5.1. Introduction

Lenovo WMI Other Mode interface is broken up into multiple GUIDs,
The primary Other Mode interface provides advanced power tuning features
such as Package Power Tracking (PPT). It is paired with multiple data block
GUIDs that provide context for the various methods.

### 5.1.1. Other Mode

WMI GUID `DC2A8805-3A8C-41BA-A6F7-092E0089CD3B`

The Other Mode WMI interface uses the firmware\_attributes class to expose
various WMI attributes provided by the interface in the sysfs. This enables
CPU and GPU power limit tuning as well as various other attributes for
devices that fall under the “Gaming Series” of Lenovo devices. Each
attribute exposed by the Other Mode interface has corresponding
capability data blocks which allow the driver to probe details about the
attribute. Each attribute has multiple pages, one for each of the platform
profiles managed by the Gamezone interface. Attributes are exposed in sysfs
under the following path:

```
/sys/class/firmware-attributes/lenovo-wmi-other/attributes/<attribute>/
```

Additionally, this driver also exports attributes to HWMON.

### 5.1.2. LENOVO\_CAPABILITY\_DATA\_00

WMI GUID `362A3AFE-3D96-4665-8530-96DAD5BB300E`

The LENOVO\_CAPABILITY\_DATA\_00 interface provides various information that
does not rely on the gamezone thermal mode.

The following HWMON attributes are implemented:
:   * fanX\_div: internal RPM divisor
    * fanX\_input: current RPM
    * fanX\_target: target RPM (tunable, 0=auto)

Due to the internal RPM divisor, the current/target RPMs are rounded down to
its nearest multiple. The divisor itself is not necessary to be a power of two.

### 5.1.3. LENOVO\_CAPABILITY\_DATA\_01

WMI GUID `7A8F5407-CB67-4D6E-B547-39B3BE018154`

The LENOVO\_CAPABILITY\_DATA\_01 interface provides various information that
relies on the gamezone thermal mode, including power limits of integrated
CPU and GPU components.

Each attribute has the following properties:
:   * current\_value
    * default\_value
    * display\_name
    * max\_value
    * min\_value
    * scalar\_increment
    * type

The following firmware-attributes are implemented:
:   * ppt\_pl1\_spl: Platform Profile Tracking Sustained Power Limit
    * ppt\_pl2\_sppt: Platform Profile Tracking Slow Package Power Tracking
    * ppt\_pl3\_fppt: Platform Profile Tracking Fast Package Power Tracking

### 5.1.4. LENOVO\_FAN\_TEST\_DATA

WMI GUID `B642801B-3D21-45DE-90AE-6E86F164FB21`

The LENOVO\_FAN\_TEST\_DATA interface provides reference data for self-test of
cooling fans.

The following HWMON attributes are implemented:
:   * fanX\_max: maximum RPM
    * fanX\_min: minimum RPM

## 5.2. WMI interface description

The WMI interface description can be decoded from the embedded binary MOF (bmof)
data using the [bmfdec](https://github.com/pali/bmfdec) utility:

```
[WMI, Dynamic, Provider("WmiProv"), Locale("MS\\0x409"), Description("LENOVO_OTHER_METHOD class"), guid("{dc2a8805-3a8c-41ba-a6f7-092e0089cd3b}")]
class LENOVO_OTHER_METHOD {
  [key, read] string InstanceName;
  [read] boolean Active;

  [WmiMethodId(17), Implemented, Description("Get Feature Value ")] void GetFeatureValue([in] uint32 IDs, [out] uint32 value);
  [WmiMethodId(18), Implemented, Description("Set Feature Value ")] void SetFeatureValue([in] uint32 IDs, [in] uint32 value);
  [WmiMethodId(19), Implemented, Description("Get Data By Command ")] void GetDataByCommand([in] uint32 IDs, [in] uint32 Command, [out] uint32 DataSize, [out, WmiSizeIs("DataSize")] uint32 Data[]);
  [WmiMethodId(99), Implemented, Description("Get Data By Package for TAC")] void GetDataByPackage([in, Max(40)] uint8 Input[], [out] uint32 DataSize, [out, WmiSizeIs("DataSize")] uint8 Data[]);
};

[WMI, Dynamic, Provider("WmiProv"), Locale("MS\\0x409"), Description("LENOVO CAPABILITY DATA 00"), guid("{362a3afe-3d96-4665-8530-96dad5bb300e}")]
class LENOVO_CAPABILITY_DATA_00 {
  [key, read] string InstanceName;
  [read] boolean Active;

  [WmiDataId(1), read, Description(" IDs.")] uint32 IDs;
  [WmiDataId(2), read, Description("Capability.")] uint32 Capability;
  [WmiDataId(3), read, Description("Capability Default Value.")] uint32 DefaultValue;
};

[WMI, Dynamic, Provider("WmiProv"), Locale("MS\\0x409"), Description("LENOVO CAPABILITY DATA 01"), guid("{7a8f5407-cb67-4d6e-b547-39b3be018154}")]
class LENOVO_CAPABILITY_DATA_01 {
  [key, read] string InstanceName;
  [read] boolean Active;

  [WmiDataId(1), read, Description(" IDs.")] uint32 IDs;
  [WmiDataId(2), read, Description("Capability.")] uint32 Capability;
  [WmiDataId(3), read, Description("Default Value.")] uint32 DefaultValue;
  [WmiDataId(4), read, Description("Step.")] uint32 Step;
  [WmiDataId(5), read, Description("Minimum Value.")] uint32 MinValue;
  [WmiDataId(6), read, Description("Maximum Value.")] uint32 MaxValue;
};

[WMI, Dynamic, Provider("WmiProv"), Locale("MS\\0x409"), Description("LENOVO CAPABILITY DATA 02"), guid("{bbf1f790-6c2f-422b-bc8c-4e7369c7f6ab}")]
class LENOVO_CAPABILITY_DATA_02 {
  [key, read] string InstanceName;
  [read] boolean Active;

  [WmiDataId(1), read, Description(" IDs.")] uint32 IDs;
  [WmiDataId(2), read, Description("Capability.")] uint32 Capability;
  [WmiDataId(3), read, Description("Data Size.")] uint32 DataSize;
  [WmiDataId(4), read, Description("Default Value"), WmiSizeIs("DataSize")] uint8 DefaultValue[];
};

[WMI, Dynamic, Provider("WmiProv"), Locale("MS\\0x409"), Description("Definition of Fan Test Data"), guid("{B642801B-3D21-45DE-90AE-6E86F164FB21}")]
class LENOVO_FAN_TEST_DATA {
  [key, read] string InstanceName;
  [read] boolean Active;
  [WmiDataId(1), read, Description("Mode.")] uint32 NumOfFans;
  [WmiDataId(2), read, Description("Fan ID."), WmiSizeIs("NumOfFans")] uint32 FanId[];
  [WmiDataId(3), read, Description("Maximum Fan Speed."), WmiSizeIs("NumOfFans")] uint32 FanMaxSpeed[];
  [WmiDataId(4), read, Description("Minumum Fan Speed."), WmiSizeIs("NumOfFans")] uint32 FanMinSpeed[];
};
```
