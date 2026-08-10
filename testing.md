These are **README badges** (often called **shields**), commonly used on GitHub, GitLab, and Bitbucket to display project metadata, status, dependencies, or hardware/software specifications. Most repository badges are generated using **[Shields.io](https://shields.io)**.

---

### How Shields.io Badges Work

The badges in your image are created using Shields.io's static badge service. The base URL structure is:

```text
[https://img.shields.io/badge/](https://img.shields.io/badge/)<LABEL>-<MESSAGE>-<COLOR>
```


#### Syntax Formatting Rules:
* **Spaces:** Replace spaces with `%20` or an underscore (`_`).
* **Dashes (`-`):** Use two dashes (`--`) if you want a literal dash inside the text.
* **Colors:** Use named colors (e.g., `blue`, `red`, `brightgreen`, `yellow`) or Hex codes (e.g., `007ec6`).

---

### Markdown Code for the Image Examples

To place these exact badges into your `README.md`, copy and paste the following Markdown code:

```markdown
![Developer](https://img.shields.io/badge/Developer-Salman%20Farsi-007ec6)
![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3%20DevKitC--1%20N16R8-red)
![Model](https://img.shields.io/badge/Model-Stories--15M%20(LLaMA--2)-brightgreen)
![License](https://img.shields.io/badge/License-MIT-yellow)
```

![Developer](https://img.shields.io/badge/Developer-Salman%20Farsi-007ec6)
![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3%20DevKitC--1%20N16R8-red)
![Model](https://img.shields.io/badge/Model-Stories--15M%20(LLaMA--2)-brightgreen)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

### Making Badges Clickable

If you want a badge to link to an external page (e.g., clicking the License badge takes the user to the license file), wrap the Markdown image syntax inside a link:

```markdown
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow)](./LICENSE)
```
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow)](./LICENSE)
---

### Customization Options

You can add query parameters to customize the visual style:

* **Style (`?style=`):** Options include `flat` (default), `flat-square`, `plastic`, `for-the-badge`, or `social`.
* **Icons (`?logo=`):** Add brand icons from [Simple Icons](https://simpleicons.org/).

**Example with square style and an icon:**
```markdown
![Python](https://img.shields.io/badge/Python-3.10-blue?style=flat-square&logo=python)
```
![Python](https://img.shields.io/badge/Python-3.10-blue?style=flat-square&logo=python)
