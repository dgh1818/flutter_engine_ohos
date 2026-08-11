# Community Changelog Standard

This standard is based on the globally adopted **Keep a Changelog 1.1.0** specification and is compatible with Semantic Versioning 2.0.0. It applies to changelog maintenance for open-source projects, community products, and technical components. The core goal is to enable users and contributors to clearly perceive key changes between versions, reducing community collaboration and upgrade costs.

## I. Core Guiding Principles

1. **Human-Centric**: Changelogs are written for humans, not machines. Dumping raw Git commit logs or internal technical implementation details is prohibited.
2. **Fully Traceable**: Every officially released version must have its own changelog entry, with no omissions of user-impacting changes.
3. **Clear Categorization**: Changes of the same type must be grouped together, maintaining consistent format and semantics.
4. **Chronological Order**: The latest version comes first, older versions follow, arranged in reverse chronological order by release date.
5. **Linkable and Navigable**: Versions, sections, and related Issues/PRs must all support anchor links for precise navigation.
6. **Transparent Compatibility**: Breaking changes, deprecated features, and security fixes must be clearly marked to inform users of risks and adaptation costs in advance.

## II. Basic File Standards

| Standard Item | Mandatory Requirement | Incorrect Example | Correct Example |
| :--- | :--- | :--- | :--- |
| Filename | Must be uppercase `CHANGELOG.md`, placed in the project repository root directory | changelog.md, ChangeLog.md, CHANGELOG.MD | CHANGELOG.md |
| File Format | Must use standard Markdown syntax, plain text only, no binary/rich-text formats | Word documents, HTML format, custom rich-text tags | Standard Markdown syntax |
| Encoding | Uniformly use UTF-8 without BOM, line endings compatible with Unix/Windows formats | GBK encoding, UTF-8 with BOM | UTF-8 without BOM |

## III. Core Structure Standard

The overall structure of the Changelog is fixed, from top to bottom: File Header → Unreleased Changes Section → Released Versions Section → Version Comparison Links Section. The order must not be rearranged arbitrarily.

### 1. File Header (Mandatory)

Located at the very top of the file, it must include 3 core elements: title, scope, and standard/version declaration. Example:

```markdown
# Changelog

All notable changes to {Project Name} will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html).
```

For projects targeting international communities, an English original declaration is required, with optional multilingual translations. If the project has special versioning rules, they should be noted here.

### 2. Unreleased Changes Section [Unreleased] (Strongly Recommended)

Located after the header and before all official versions, used to collect in-development, unreleased changes to avoid last-minute changelog compilation before release.

```markdown
## [Unreleased]

### Added
- Added XX feature module with XX capability

### Fixed
- Fixed crash issue in XX scenario
```

When releasing, simply rename [Unreleased] to the corresponding version number + release date, and add a new blank [Unreleased] section below. Breaking changes and deprecation plans for upcoming releases can be noted here in advance.

### 3. Released Versions Section (Mandatory)

This is the core body of the Changelog. Each independent version corresponds to a level-2 heading block, strictly following the format:

```markdown
## [Version Number] - Release Date
```

- Version Number: Strictly follows Semantic Versioning MAJOR.MINOR.PATCH, e.g., 1.2.0, 2.0.0. The prefix "v" or other custom characters are prohibited.
- Release Date: Must use ISO 8601 standard format YYYY-MM-DD, e.g., 2026-04-09. Abbreviations and non-standard separators are prohibited.

Within each version block, changes must be grouped by type using level-3 headings, with same-type changes recorded as unordered list items.

### 4. Version Comparison Links Section (Mandatory)

Located at the bottom of the file, add diff comparison links for each version to help users view the full code changes. Format example:

```markdown
[Unreleased]: https://gitcode.com/{org}/{repo}/compare/v1.2.0...HEAD
[1.2.0]: https://gitcode.com/{org}/{repo}/compare/v1.1.0...v1.2.0
[1.1.0]: https://gitcode.com/{org}/{repo}/compare/v1.0.0...v1.1.0
[1.0.0]: https://gitcode.com/{org}/{repo}/releases/tag/v1.0.0
```

Links must correspond one-to-one with version numbers and must be accessible. The first official version should link directly to the Release page, without a comparison link.

## IV. Change Type Classification Standard

Change types are divided into 7 standard mandatory types and 1 community-extended optional type. They must be strictly ordered by priority. The order and type names must not be rearranged or customized, ensuring unified community understanding.

### 1. Standard Mandatory Types (Priority from High to Low)

| Type | Applicable Scenarios | Writing Guidelines |
| :--- | :--- | :--- |
| Added | New features, capabilities, modules, APIs, configuration options, dependencies, etc. | Clearly state the core capability, applicable scenarios, and the value users gain |
| Changed | Modifications to existing feature logic, behavior, UI, API input/output parameters, dependency versions, default configurations | Clearly describe the "old behavior → new behavior" change. Breaking changes must be highlighted with [BREAKING CHANGE] at the beginning of the entry, with adaptation guidance |
| Deprecated | Features, APIs, configuration options planned for removal in future versions | State the deprecation reason, planned removal version, recommend alternatives, and remind users to adapt in advance |
| Removed | Features, APIs, configuration options, dependencies officially removed in this version (previously marked as deprecated) | Clearly state what was removed, the impact scope, and provide complete alternatives and upgrade steps |
| Fixed | Fixes for various bugs, exceptions, and compatibility issues | Clearly describe the problem scenario, impact scope, and fix result, linking the corresponding Issue/PR number |
| Security | Fixes and improvements related to security vulnerabilities and privacy compliance | State the vulnerability severity, affected versions, and fix result, linking CVE numbers and security advisory links, reminding users to upgrade urgently |
| Documentation | Documentation changes that impact users, such as new tutorials, best practices, API documentation completion, major errata, etc. | Clearly state the documentation change content and its impact on users, such as new usage guides, API documentation improvements, etc. |

### 2. Community-Extended Optional Type

Only used when the change cannot be classified into the above 7 types and has a clear impact on users. Abuse of extended types to override standard types is prohibited:

- **Performance**: Performance optimizations that do not change functional logic, such as API response speed, memory usage, startup speed optimization, etc.

## V. Entry Writing Standards

### Single Entry Requirements

Each change entry should be limited to 1-2 sentences, concise and complete, using declarative sentences. Avoid vague or meaningless descriptions.

❌ Incorrect Example: Optimized some experience, fixed some bugs, improved performance

✅ Correct Example: Optimized list pagination loading speed, peak loading time reduced by 40%; fixed image upload failure on OpenHarmony 6.0, related to Issue #123

### Perspective Requirements

Always write from the perspective of community users/contributors, focusing on "the impact of the change on users" rather than internal technical implementation details.

❌ Incorrect Example: Refactored the code structure of XX class, adjusted the naming of XX variable

✅ Correct Example: Refactored the underlying logic of XX module, API call success rate improved from 99.2% to 99.95%, no API usage changes

### Reference Standards

In community collaboration scenarios, it is recommended to link the corresponding PR number, Issue number, and contributor ID at the end of each change entry to improve traceability and respect community contributions.

Example: Added batch user role permission management feature, by @contributor-name, PR #456

### Breaking Change Standards

1. Must be highlighted with [BREAKING CHANGE] at the very beginning of the change entry, not hidden among regular entries
2. Breaking changes in major version (MAJOR) upgrades require a separate "Breaking Changes" section below the version heading for centralized explanation
3. Must include a complete upgrade and adaptation guide, clearly stating old approach → new approach replacement steps to reduce user upgrade costs

## VI. Community Maintenance Best Practices

1. **Update in Sync with Development**: Update Changelog entries when PRs are merged, rather than batch-writing before release, to avoid missing key changes and reduce maintenance costs.
2. **Distinguish "User-Visible" from "Internal Changes"**: Only record changes that impact community users and downstream consumers. No need to record internal code formatting, CI/CD configuration adjustments, minor dev dependency upgrades, or other imperceptible changes.
3. **Release Consistency**: Version numbers and release dates recorded in the Changelog must exactly match the repository Release tags and package management platform release versions.
4. **Multilingual Adaptation**: For projects targeting multilingual communities, multilingual versions such as CHANGELOG-zh-CN.md and CHANGELOG-en.md can be provided, ensuring core change content is fully consistent.
5. **Community Outreach**: When releasing a version, sync the corresponding Changelog content to Release notes, community announcements, user groups, and other channels to improve change reach.

## VII. Complete Standard Template

```markdown
# Changelog

All notable changes to Demo-Project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Added batch data export to Excel with custom field filtering
- Added Webhook callback capability with event-triggered real-time push

### Deprecated
- Deprecated legacy `/v1/user/list` API, planned for removal in version 2.0.0, use `/v2/user/page` instead

## [2.0.0] - 2026-03-15

### Breaking Changes
1. Refactored user authentication API signature algorithm from MD5 to SHA256, old signature method is no longer compatible
2. Removed deprecated configuration option `timeout_old`, replace with `request_timeout`

### Added
- Added user two-factor authentication with SMS/email verification code dual-channel support, by @community-contributor, PR #234
- Added dark mode theme with automatic system theme switching

### Changed
- Optimized homepage list rendering logic, first-screen loading speed improved by 30%

### Fixed
- Fixed abnormal response when pagination query page number exceeds range, related to Issue #189
- Fixed form submit button unresponsive on Firefox browser compatibility issue

### Security
- Fixed high-severity unauthorized user information access vulnerability, affected versions: 1.0.0-1.1.0, all users are advised to upgrade to this version
- Upgraded sensitive dependency `axios` to version 1.7.9, fixing known security vulnerabilities

## [1.1.0] - 2026-02-10

### Added
- Added user profile editing functionality
- Added operation log query capability with time and operation type filtering

### Fixed
- Fixed issue where automatic redirect to login page failed after login session expiration
- Fixed abnormal data statistics chart value display issue

## [1.0.0] - 2026-01-01

### Added
- Project officially released with core features including user management, data querying, and report generation
- Accompanied by official usage documentation and API reference documentation

[Unreleased]: https://gitcode.com/{org}/{repo}/compare/v2.0.0...HEAD
[2.0.0]: https://gitcode.com/{org}/{repo}/compare/v1.1.0...v2.0.0
[1.1.0]: https://gitcode.com/{org}/{repo}/compare/v1.0.0...v1.1.0
[1.0.0]: https://gitcode.com/{org}/{repo}/releases/tag/v1.0.0
```
