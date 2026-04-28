# Changelog

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
