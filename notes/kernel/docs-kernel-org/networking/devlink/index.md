# Linux Devlink Documentation

> 출처(원문): https://docs.kernel.org/networking/devlink/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Devlink Documentation

devlink is an API to expose device information and resources not directly
related to any device class, such as chip-wide/switch-ASIC-wide configuration.

## Locking

Driver facing APIs are currently transitioning to allow more explicit
locking. Drivers can use the existing `devlink_*` set of APIs, or
new APIs prefixed by `devl_*`. The older APIs handle all the locking
in devlink core, but don’t allow registration of most sub-objects once
the main devlink object is itself registered. The newer `devl_*` APIs assume
the devlink instance lock is already held. Drivers can take the instance
lock by calling `devl_lock()`. It is also held all callbacks of devlink
netlink commands.

Drivers are encouraged to use the devlink instance lock for their own needs.

Drivers need to be cautious when taking devlink instance lock and
taking RTNL lock at the same time. Devlink instance lock needs to be taken
first, only after that RTNL lock could be taken.

## Nested instances

Some objects, like linecards or port functions, could have another
devlink instances created underneath. In that case, drivers should make
sure to respect following rules:

> * Lock ordering should be maintained. If driver needs to take instance
>   lock of both nested and parent instances at the same time, devlink
>   instance lock of the parent instance should be taken first, only then
>   instance lock of the nested instance could be taken.
> * Driver should use object-specific helpers to setup the
>   nested relationship:
>
>   + `devl_nested_devlink_set()` - called to setup devlink -> nested
>     devlink relationship (could be user for multiple nested instances.
>   + `devl_port_fn_devlink_set()` - called to setup port function ->
>     nested devlink relationship.
>   + `devlink_linecard_nested_dl_set()` - called to setup linecard ->
>     nested devlink relationship.

The nested devlink info is exposed to the userspace over object-specific
attributes of devlink netlink.

## Interface documentation

The following pages describe various interfaces available through devlink in
general.

* [Devlink DPIPE](devlink-dpipe.html)
* [Devlink E-Switch Attribute](devlink-eswitch-attr.html)
* [Devlink Flash](devlink-flash.html)
* [Devlink Health](devlink-health.html)
* [Devlink Info](devlink-info.html)
* [Devlink Line card](devlink-linecard.html)
* [Devlink Params](devlink-params.html)
* [Devlink Port](devlink-port.html)
* [Devlink Region](devlink-region.html)
* [Devlink Reload](devlink-reload.html)
* [Devlink Resource](devlink-resource.html)
* [Devlink Selftests](devlink-selftests.html)
* [Devlink Trap](devlink-trap.html)
* [Devlink Shared Instances](devlink-shared.html)

## Driver-specific documentation

Each driver that implements `devlink` is expected to document what
parameters, info versions, and other features it supports.

* [am65-cpsw-nuss devlink support](am65-nuss-cpsw-switch.html)
* [bnxt devlink support](bnxt.html)
* [etas\_es58x devlink support](etas_es58x.html)
* [hns3 devlink support](hns3.html)
* [i40e devlink support](i40e.html)
* [ice devlink support](ice.html)
* [ionic devlink support](ionic.html)
* [iosm devlink support](iosm.html)
* [ixgbe devlink support](ixgbe.html)
* [kvaser\_pciefd devlink support](kvaser_pciefd.html)
* [kvaser\_usb devlink support](kvaser_usb.html)
* [mlx4 devlink support](mlx4.html)
* [mlx5 devlink support](mlx5.html)
* [mlxsw devlink support](mlxsw.html)
* [mv88e6xxx devlink support](mv88e6xxx.html)
* [netdevsim devlink support](netdevsim.html)
* [nfp devlink support](nfp.html)
* [octeontx2 devlink support](octeontx2.html)
* [prestera devlink support](prestera.html)
* [qed devlink support](qed.html)
* [sfc devlink support](sfc.html)
* [stmmac (synopsys dwmac) devlink support](stmmac.html)
* [ti-cpsw-switch devlink support](ti-cpsw-switch.html)
* [zl3073x devlink support](zl3073x.html)
