# Full Moon Editor REST API

Run `make edit` to run our data editor. Source is at `src/tool/editor`.

Any path beginning "/api/" is a dynamic API call, see below.

Any path beginning "/data/" is a verbatim access to data files under `src/data`.
GET, PUT, and DELETE are permitted.
GET a directory for a JSON array of basenames under it.

Static files under `src/tool/editor/www` are served verbatim, GET only.

## API Calls

`GET /api/spriteControllers`

Returns JSON:

```
[{
  name: string
  id: number
}, ...]
```
