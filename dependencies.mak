# this is used by sipxbuild to compute a correct build order

.PHONY: sipXfreeSwitch
sipXfreeSwitch :
	@echo sipXfreeSwitch

.PHONY: doc
doc : sipXcommserverLib
	@echo doc

.PHONY: sipXtools
sipXtools : sipXcommons sipXtackLib 
	@echo sipXtools

.PHONY: sipXbuild
sipXbuild :
	@echo sipXbuild

.PHONY: sipXportLib
sipXportLib :
	@echo sipXportLib

.PHONY: sipXtackLib
sipXtackLib : sipXportLib
	@echo sipXtackLib

.PHONY: sipXmediaLib
sipXmediaLib : sipXtackLib
	@echo sipXmediaLib

.PHONY: sipXmediaAdapterLib
sipXmediaAdapterLib : sipXmediaLib
	@echo sipXmediaAdapterLib

.PHONY: sipXcallLib
sipXcallLib : sipXmediaAdapterLib
	@echo sipXcallLib

.PHONY: sipXcommserverLib
sipXcommserverLib : sipXtackLib
	@echo sipXcommserverLib


.PHONY: sipXcommons
sipXcommons :
	@echo sipXcommons

.PHONY: sipXpublisher
sipXpublisher : sipXcommserverLib
	@echo sipXpublisher

.PHONY: sipXregistry
sipXregistry : sipXcommserverLib
	@echo sipXregistry

.PHONY: sipXprovision
sipXprovision : sipXcommons
	@echo sipXprovision

.PHONY: sipXproxy
sipXproxy : sipXcommserverLib
	@echo sipXproxy

.PHONY: sipXconfig
sipXconfig : sipXcommons
	@echo sipXconfig

.PHONY: sipXacd
sipXacd : sipXcallLib
	@echo sipXacd

.PHONY: sipXpark
sipXpark : sipXcallLib sipXcommserverLib sipXmediaAdapterLib
	@echo sipXpark

.PHONY: sipXpresence
sipXpresence : sipXcallLib sipXcommserverLib sipXmediaAdapterLib
	@echo sipXpresence

.PHONY: sipXrls
sipXrls : sipXcallLib sipXcommserverLib sipXmediaAdapterLib
	@echo sipXrls

.PHONY: sipXpbx
sipXpbx : sipXproxy sipXprovision sipXregistry sipXpublisher sipXconfig sipXpark sipXpresence sipXrls
	@echo sipXpbx

.PHONY: sipXsupervisor
sipXsupervisor : sipXcommserverLib sipXpbx
	@echo sipXsupervisor

.PHONY: sipXrelay
sipXrelay : sipXcommons
	@echo sipXrelay

.PHONY: sipXbridge
sipXbridge : sipXcommons sipXrelay
	@echo sipXbridge


.PHONY: sipXpage
sipXpage : sipXcommons
	@echo sipXpage

.PHONY: sipXivr
sipXivr : sipXcommons sipXopenfire
	@echo sipXivr

.PHONY: sipXsaa
sipXsaa : sipXcommserverLib
	@echo sipXsaa

.PHONY: sipXopenfire
sipXopenfire : sipXcommons
	@echo sipXopenfire

.PHONY: sipXrecording
sipXrecording: sipXcommons sipXopenfire
	@echo sipXrecording

.PHONY: sipXrest
sipXrest : sipXcommons 
	@echo sipXrest

.PHONY: sipXcallController
sipXcallController : sipXrest 
	@echo sipXcallController

.PHONY: sipXcdrLog
sipXcdrLog : sipXrest 
	@echo sipXcdrLog

.PHONY: sipXecs
sipXecs : \
	sipXcallController \
	sipXcdrLog \
	sipXconfig \
	sipXfreeSwitch \
	sipXivr \
	sipXopenfire \
	sipXpage \
	sipXpark \
	sipXpbx \
	sipXpresence \
	sipXprovision \
	sipXproxy \
	sipXpublisher \
	sipXregistry \
	sipXrest \
        sipXrelay \
        sipXbridge \
	sipXrls \
	sipXsaa \
	sipXsupervisor \
	sipXtools \
	doc
	@echo sipXecs
