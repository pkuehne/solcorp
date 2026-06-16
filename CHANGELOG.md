# Changelog

## [0.2.0](https://github.com/pkuehne/solcorp/compare/v0.1.0...v0.2.0) (2026-06-16)


### Features

* **config:** Load font name and size from config.lua  ([#190](https://github.com/pkuehne/solcorp/issues/190)) ([2510a5a](https://github.com/pkuehne/solcorp/commit/2510a5a4cbc64b59c73828aab9eec3d1ab4c48af)), closes [#89](https://github.com/pkuehne/solcorp/issues/89)
* **rocket:** Add Contracts Detail Window ([#204](https://github.com/pkuehne/solcorp/issues/204)) ([f4d4713](https://github.com/pkuehne/solcorp/commit/f4d47135c8bd0de12fc90dc63757518406f816a3))
* **rocket:** Add states to Launch Plans ([#199](https://github.com/pkuehne/solcorp/issues/199)) ([4f76179](https://github.com/pkuehne/solcorp/commit/4f761790a334e0d9c327def0d07a4e8dd0cdd167))
* **rocket:** Rocket List & Detail Windows ([#203](https://github.com/pkuehne/solcorp/issues/203)) ([0acc50e](https://github.com/pkuehne/solcorp/commit/0acc50e027e223b5838de0cb200a82fca474129c))
* **rocket:** Rockets can now be set to automatically move to storage after being built ([#177](https://github.com/pkuehne/solcorp/issues/177)) ([5c68455](https://github.com/pkuehne/solcorp/commit/5c6845523fcb6cb656096435a7d682367544ac23))
* **rocket:** When a manufacturing line is done it can automatically ([5c68455](https://github.com/pkuehne/solcorp/commit/5c6845523fcb6cb656096435a7d682367544ac23))
* **site:** Add weather system ([#209](https://github.com/pkuehne/solcorp/issues/209)) ([9b7f835](https://github.com/pkuehne/solcorp/commit/9b7f835ad8ef3c364a8d1743817bf6f8895fd683))
* **toolbar:** Add Toolbar with notification button ([#188](https://github.com/pkuehne/solcorp/issues/188)) ([45af802](https://github.com/pkuehne/solcorp/commit/45af802b447ac93722371bb2a81d082f2f0d79c7)), closes [#104](https://github.com/pkuehne/solcorp/issues/104)
* **widget:** format monetary amounts ([#183](https://github.com/pkuehne/solcorp/issues/183)) ([c20ae77](https://github.com/pkuehne/solcorp/commit/c20ae77c80d3189c6f6df389f916fd4f5286f516)), closes [#124](https://github.com/pkuehne/solcorp/issues/124)
* **window:** Notification Window ([#182](https://github.com/pkuehne/solcorp/issues/182)) ([dc986bb](https://github.com/pkuehne/solcorp/commit/dc986bbb36807dbdbf2ef6c90fa5a9e0764b3b98)), closes [#75](https://github.com/pkuehne/solcorp/issues/75)


### Bug Fixes

* allow unlimited contracts and guard against duplicate contract payloads ([#197](https://github.com/pkuehne/solcorp/issues/197)) ([4a54e32](https://github.com/pkuehne/solcorp/commit/4a54e323cf419f5572a50df49d913da284caae18))


### Refactoring

* **lint:** Remove all bugprone-type clang-tidy exceptions ([#162](https://github.com/pkuehne/solcorp/issues/162)) ([dfc5408](https://github.com/pkuehne/solcorp/commit/dfc5408665eb0c0b7fe58f5c789abf96568c5c6f)), closes [#140](https://github.com/pkuehne/solcorp/issues/140)
* **lint:** Remove all modernize clang tidy exceptions ([#163](https://github.com/pkuehne/solcorp/issues/163)) ([5fb04d5](https://github.com/pkuehne/solcorp/commit/5fb04d5ae712fae36321867fbeba694fff01a82f)), closes [#149](https://github.com/pkuehne/solcorp/issues/149)
* **lint:** Remove performance-type clang-tidy exceptions ([#164](https://github.com/pkuehne/solcorp/issues/164)) ([a144bb8](https://github.com/pkuehne/solcorp/commit/a144bb816888b0978c74da6da451717cd139771e))
* **lint:** Remove some egregious clang tidy suppressions ([#160](https://github.com/pkuehne/solcorp/issues/160)) ([ba21bbd](https://github.com/pkuehne/solcorp/commit/ba21bbd598e2bbca5c2f2c765261fd2b6de59cb3)), closes [#139](https://github.com/pkuehne/solcorp/issues/139)
* **lua:** Replace sol3 with plain lua C API ([#127](https://github.com/pkuehne/solcorp/issues/127)) ([22253f2](https://github.com/pkuehne/solcorp/commit/22253f2c64e2df86ffc77a844aade93afe562f6f)), closes [#32](https://github.com/pkuehne/solcorp/issues/32)
* **modules:** Complete renaming of rocket and main modules ([#166](https://github.com/pkuehne/solcorp/issues/166)) ([538e322](https://github.com/pkuehne/solcorp/commit/538e322d2c3a9becb26a4c20037667fd74e2720c))
* **modules:** Rename rocket_launch and main_menu modules ([#161](https://github.com/pkuehne/solcorp/issues/161)) ([89d2b42](https://github.com/pkuehne/solcorp/commit/89d2b4280d746df081a7746b012e3d15677f76bf)), closes [#106](https://github.com/pkuehne/solcorp/issues/106)
* **notification:** Add dates ([#208](https://github.com/pkuehne/solcorp/issues/208)) ([6a74edd](https://github.com/pkuehne/solcorp/commit/6a74eddbab8b259d212faa96fc2aa307e0e89b32))
* **rocket:** Moving rockets is now a two step process ([#172](https://github.com/pkuehne/solcorp/issues/172)) ([2898355](https://github.com/pkuehne/solcorp/commit/2898355480d56575930366e49a5b2fa49c0bc517)), closes [#93](https://github.com/pkuehne/solcorp/issues/93)
* **rocket:** Retain rockets in orbit after launch ([#212](https://github.com/pkuehne/solcorp/issues/212)) ([ed65d10](https://github.com/pkuehne/solcorp/commit/ed65d1061c847ceed9e01f39b9329895d7f76e6c))
* **rocket:** Rocket state machine uses entities via relationship instead of enums ([#191](https://github.com/pkuehne/solcorp/issues/191)) ([f0c24a5](https://github.com/pkuehne/solcorp/commit/f0c24a5c49d88a30d2d054ea336cebc69887b1f8)), closes [#178](https://github.com/pkuehne/solcorp/issues/178)
* **rocket:** Rockets now have dedicated state ([#165](https://github.com/pkuehne/solcorp/issues/165)) ([3da5edb](https://github.com/pkuehne/solcorp/commit/3da5edb234dc6d7ca5897787314870d6fc403282)), closes [#91](https://github.com/pkuehne/solcorp/issues/91) [#92](https://github.com/pkuehne/solcorp/issues/92)
* **stat:** Centrally registered stats ([#211](https://github.com/pkuehne/solcorp/issues/211)) ([63db268](https://github.com/pkuehne/solcorp/commit/63db2686980f597bc8d7cd6bb3e0dcce112d15e8))
* **window:** Rename "main" module into "window" module and re-organize tests ([#176](https://github.com/pkuehne/solcorp/issues/176)) ([adcd1a4](https://github.com/pkuehne/solcorp/commit/adcd1a47b40edd20bff5c4ef77448219f11b7a62))


### Chores

* **base:** Add notification infrastructure ([#181](https://github.com/pkuehne/solcorp/issues/181)) ([2aa52be](https://github.com/pkuehne/solcorp/commit/2aa52be868b7a957a723df57dc8926a872156fc7))
* **ci:** Replace nix cache image in Github Actions ([#117](https://github.com/pkuehne/solcorp/issues/117)) ([7bfa687](https://github.com/pkuehne/solcorp/commit/7bfa6873513b54277ddead3407ed6bc21e8e12d3))
* **toolchain:** Bump Nix to 25.11 ([#159](https://github.com/pkuehne/solcorp/issues/159)) ([4628eb3](https://github.com/pkuehne/solcorp/commit/4628eb3f1ffcd7d47cf5b3d5b542f176a08edeac)), closes [#132](https://github.com/pkuehne/solcorp/issues/132)

## [0.1.0](https://github.com/pkuehne/solcorp/compare/v0.0.1...v0.1.0) (2026-04-28)


### Features

* Add a basic balance that's updated by builds and contracts ([#78](https://github.com/pkuehne/solcorp/issues/78)) ([736def5](https://github.com/pkuehne/solcorp/commit/736def5633a8e940c4ab8ab35ced3019e84e0a6b))
* add Lua linting and formatting CI checks ([#62](https://github.com/pkuehne/solcorp/issues/62)) ([a741b2e](https://github.com/pkuehne/solcorp/commit/a741b2ea6b8ffad5914fc3c340ef9fc2e562cb34))
* **build:** release includes all files ([#90](https://github.com/pkuehne/solcorp/issues/90)) ([0fd06ec](https://github.com/pkuehne/solcorp/commit/0fd06ec3aabd27ab98f59f11a9d81a81f4c27a90)), closes [#77](https://github.com/pkuehne/solcorp/issues/77)
* **build:** Windows Builds! ([#85](https://github.com/pkuehne/solcorp/issues/85)) ([c35382f](https://github.com/pkuehne/solcorp/commit/c35382f87dfd2f742b8f0bfb062a9bf2e4a1af5d)), closes [#81](https://github.com/pkuehne/solcorp/issues/81)
* **docs:** Add 003-finance-system ADR ([a38148b](https://github.com/pkuehne/solcorp/commit/a38148bd48a31358ad76e919fb00c24757da31d4))
* **docs:** Add 004-action-pattern ADR and fix action header ([a38148b](https://github.com/pkuehne/solcorp/commit/a38148bd48a31358ad76e919fb00c24757da31d4))
* **docs:** Add 005-game-calendar ([a38148b](https://github.com/pkuehne/solcorp/commit/a38148bd48a31358ad76e919fb00c24757da31d4))
* **docs:** Add Design docs ([#68](https://github.com/pkuehne/solcorp/issues/68)) ([a38148b](https://github.com/pkuehne/solcorp/commit/a38148bd48a31358ad76e919fb00c24757da31d4))
* **docs:** Add design document for personnel management ([a38148b](https://github.com/pkuehne/solcorp/commit/a38148bd48a31358ad76e919fb00c24757da31d4))
* **docs:** documentation ([#67](https://github.com/pkuehne/solcorp/issues/67)) ([060022e](https://github.com/pkuehne/solcorp/commit/060022edf4d8219d304ef4c411b90b192e19d459))
* **lua:** clean up core mod ([#101](https://github.com/pkuehne/solcorp/issues/101)) ([508ac82](https://github.com/pkuehne/solcorp/commit/508ac826967c6309b781c5243c51450ace35a2e5))
* **rockets:** Adds a new window to show all active launches ([8f9bb50](https://github.com/pkuehne/solcorp/commit/8f9bb50c8a1ce6f53f2e0cfff3a00c48f38b1723))
* **rockets:** Adds a window to show all open and closed contracts ([8f9bb50](https://github.com/pkuehne/solcorp/commit/8f9bb50c8a1ce6f53f2e0cfff3a00c48f38b1723))
* **rockets:** Adds Contracts that can be created from lua ([8f9bb50](https://github.com/pkuehne/solcorp/commit/8f9bb50c8a1ce6f53f2e0cfff3a00c48f38b1723))
* **rockets:** Contract Launches ([#63](https://github.com/pkuehne/solcorp/issues/63)) ([8f9bb50](https://github.com/pkuehne/solcorp/commit/8f9bb50c8a1ce6f53f2e0cfff3a00c48f38b1723))
* **rockets:** failure rate for rockets ([#86](https://github.com/pkuehne/solcorp/issues/86)) ([d9f94b9](https://github.com/pkuehne/solcorp/commit/d9f94b98e6b95add0623c215daad566674f3d84b)), closes [#74](https://github.com/pkuehne/solcorp/issues/74)
* **rockets:** Launch Plans now include a target orbit and optionally payloads assigned through accepted contracts ([8f9bb50](https://github.com/pkuehne/solcorp/commit/8f9bb50c8a1ce6f53f2e0cfff3a00c48f38b1723))
* **rockets:** Select a rocket prefab to when building from a manufacturing line ([#109](https://github.com/pkuehne/solcorp/issues/109)) ([083a2a3](https://github.com/pkuehne/solcorp/commit/083a2a3cd0de04fa8de2955a6f3d70a8c42627e7))


### Bug Fixes

* add clang-format config, fix formatting, add CI format check ([23b40aa](https://github.com/pkuehne/solcorp/commit/23b40aa004274fc23e623e8f56affa6aead5f9cb))
* add clang-format config, fix formatting, and add CI format check ([#58](https://github.com/pkuehne/solcorp/issues/58)) ([23b40aa](https://github.com/pkuehne/solcorp/commit/23b40aa004274fc23e623e8f56affa6aead5f9cb))
* **build:** Ensure cmake Debug builds work ([#66](https://github.com/pkuehne/solcorp/issues/66)) ([83d2138](https://github.com/pkuehne/solcorp/commit/83d213854ded7cb6a221149e9670743f484fbf09))
* **rockets:** Clear launch plan details on cancel ([8f9bb50](https://github.com/pkuehne/solcorp/commit/8f9bb50c8a1ce6f53f2e0cfff3a00c48f38b1723))


### Chores

* **build:** upgrade to c++ 20 ([#71](https://github.com/pkuehne/solcorp/issues/71)) ([36d42fa](https://github.com/pkuehne/solcorp/commit/36d42fa518ce7d03625832b937542e57ad3c8e03))
* bump actions/checkout from 4 to 6 ([#80](https://github.com/pkuehne/solcorp/issues/80)) ([6eb3844](https://github.com/pkuehne/solcorp/commit/6eb3844eed8b8058c780e7af8452ce170687f57d))
* bump actions/upload-pages-artifact from 3 to 5 ([#79](https://github.com/pkuehne/solcorp/issues/79)) ([91c109d](https://github.com/pkuehne/solcorp/commit/91c109d3025c8799475aeb10a50faf8f864c09e3))
* bump googleapis/release-please-action from 4 to 5 ([#108](https://github.com/pkuehne/solcorp/issues/108)) ([60927ea](https://github.com/pkuehne/solcorp/commit/60927ea780016a7772019597baa0bf9644c986ac))
* bump softprops/action-gh-release from 2 to 3 ([#107](https://github.com/pkuehne/solcorp/issues/107)) ([1cab190](https://github.com/pkuehne/solcorp/commit/1cab1901b287c6f0432b9ccdfead9c831e59c7a3))
* configure dependabot to use conventional commit messages ([#56](https://github.com/pkuehne/solcorp/issues/56)) ([dd153b0](https://github.com/pkuehne/solcorp/commit/dd153b00c21bab3bdc53b7adeab71274b584e5ec))
