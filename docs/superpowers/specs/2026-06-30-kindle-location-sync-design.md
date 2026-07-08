# Kindle Location Sync Design

## Goal

Add an EPUB reader workflow that lets a CrossPoint user manually sync reading position with a Kindle using the Kindle location number shown on the Kindle.

This is not an attempt to reproduce Amazon's internal MOBI/AZW/KFX location algorithm from the EPUB file. The practical goal is bidirectional manual rendezvous:

- Kindle to CrossPoint: enter a Kindle location and jump to the corresponding EPUB position.
- CrossPoint to Kindle: show the estimated Kindle location for the current EPUB position.

The first version should work well enough after a one-time per-book setup: the user enters the Kindle total location count, such as `Loc 1234 of 8450`, and CrossPoint stores `8450` for that book.

## Context

CrossPoint reads EPUB by spine item and page number. Existing EPUB progress is stored as `spineIndex`, `pageNumber`, and `pageCount`. Existing percent navigation maps an integer percent to an absolute position across EPUB spine sizes. Integer percent is too coarse for long books because one percent can span many pages.

Kindle locations are finer grained and easier to use for manual sync than percent. However, Kindle computes locations after its own conversion pipeline, while CrossPoint reads the original EPUB. Exact compatibility is therefore unrealistic without a sidecar generated from the same Kindle conversion output.

KOReader/KOSync is useful prior art, but it is not a user-facing location system. KOSync stores an opaque progress cursor plus a percentage. CrossPoint should keep KOSync separate from this feature.

## Proposed First Slice

Add a per-book "Kindle total locations" setting for EPUB books.

Once set, CrossPoint computes an estimated Kindle location from its current EPUB progress:

```text
estimatedKindleLocation = round(epubProgressRatio * kindleTotalLocations)
```

And jumps from an entered Kindle location by reversing the same scale:

```text
targetProgressRatio = enteredKindleLocation / kindleTotalLocations
```

The progress ratio should be based on a stable EPUB reading-position model, not the visible percent integer. The initial implementation can use existing book/spine size progress if visible-text indexing proves too large for the first PR, but the preferred model is visible text progress across the EPUB spine:

```text
visibleTextOffsetAcrossBook / totalVisibleTextAcrossBook
```

The UI should label these values as Kindle locations or estimated Kindle locations, not as native CrossPoint locations, so the user understands the feature is for manual Kindle sync.

## User Flow

Reader menu gains a Kindle location entry point when reading an EPUB:

- If no total has been configured, selecting the entry opens a numeric input to set "Kindle total locations".
- If a total has been configured, the menu shows the current estimate, for example `Kindle Loc 1234 / 8450`.
- The user can choose "Go to Kindle location" and enter the number visible on the Kindle.
- The user can edit or clear the stored total from the same flow.

This makes setup annoying only once per book, which is acceptable because on-device input is slow.

## Data Storage

Store the per-book Kindle total in a small sidecar file under the book's existing cache/progress area. Do not change `progress.bin`; existing reading progress must remain backward-compatible.

The stored data should include:

- Kindle total location count.
- A small version marker.
- Enough book identity context to avoid applying a stale total after cache reuse or book replacement, using whatever identifier CrossPoint already uses for per-book cache/progress paths.

Calibration anchors are explicitly out of scope for the first PR, but the file format should leave room to add them later.

## Components

### Location Model

Create a small EPUB Kindle-location helper rather than embedding this logic directly in `EpubReaderActivity`.

Responsibilities:

- Validate a configured total location count.
- Convert EPUB progress ratio to Kindle location.
- Convert Kindle location to target progress ratio.
- Clamp locations to `1..total`.

### Reader Activity Integration

`EpubReaderActivity` should reuse the existing percent jump path by introducing a shared internal jump by normalized progress or absolute book position. `jumpToPercent()` can call the shared path, and the new Kindle-location flow can call it with finer precision.

### UI

Use numeric entry controls already present in CrossPoint where possible. The first implementation should prefer simple numeric screens over a new complex editor.

Minimal strings:

- `Kindle location`
- `Set Kindle total`
- `Go to Kindle location`
- `Clear Kindle total`
- `Kindle Loc %d / %d`

## Error Handling

- If the total is missing, prompt for total before allowing jumps.
- If the total is zero, negative, or too large for the UI type, reject it with a simple error message.
- If the user enters a target location less than `1`, clamp to `1`.
- If the user enters a target location greater than the configured total, clamp to the configured total.
- If the sidecar cannot be read, continue without a configured total.
- If the sidecar cannot be written, keep the current session value but show that it was not saved if the UI has an existing error pattern for this.

## Testing

Add focused tests for the pure location model:

- progress `0.0` maps to location `1`.
- progress `1.0` maps to the configured total.
- middle progress maps to the expected rounded location.
- entered locations clamp to `1..total`.
- invalid totals are rejected.

For integration, validate manually on device or emulator-equivalent build:

- configure Kindle total for an EPUB.
- close and reopen the book and confirm total persists.
- jump to several Kindle locations, including start/end.
- confirm existing percent jump still behaves as before.

## Deferred Work

### Calibration Anchors

If simple scaling is not precise enough, add optional anchors:

```text
Kindle location N = current CrossPoint EPUB position
```

CrossPoint can then interpolate between anchors instead of using a single global scale. This would handle front matter differences, Kindle conversion quirks, and mismatched EPUB editions better.

### Desktop Sidecar

A future desktop helper could generate a richer mapping file by comparing an EPUB-derived reading stream with a Kindle-converted file. This should not be required for the first firmware PR.

### Status Bar Mode

The first PR can show Kindle location in the reader menu. A status bar display mode can be added later if it does not conflict with existing page/status PRs.

## Open Decision

The first implementation should prefer visible-text progress if it can be implemented without heavy memory or startup cost. If visible-text indexing makes the PR too large, use existing spine-size progress for the first PR and make visible-text progress a follow-up.
