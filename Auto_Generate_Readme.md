# Auto-Generate README Index Files in GitHub

Perfect — here is the index version you can create 100% in the browser.

It will create a `README.md` in every folder that doesn't have one, listing all the `.md` files inside.

## Do this in github.com

### 1. Create the workflow file

- Go to your repo > **Add file > Create new file**
- In the filename box, type:

```text
.github/workflows/auto-readme.yml
```

- Paste this entire code:

```yaml
name: Auto Generate README Index
on:
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - uses: actions/checkout@v5

      - name: Generate index READMEs
        run: |
          find . -type f -name "*.md" ! -iname "readme.md" -exec dirname {} \; | sort -u | while IFS= read -r dir; do
            # skip .github and .git folders
            if [[ "$dir" == *".github"* ]]; then continue; fi
            if [ -f "$dir/README.md" ] || [ -f "$dir/readme.md" ]; then continue; fi

            echo "Generating $dir/README.md"
            echo "# $(basename "$dir")" > "$dir/README.md"
            echo "" >> "$dir/README.md"
            echo "Documents in this folder:" >> "$dir/README.md"
            echo "" >> "$dir/README.md"

            for md in "$dir"/*.md; do
              [ -e "$md" ] || continue
              fname=$(basename "$md")
              # skip readme itself
              if [[ "${fname,,}" == "readme.md" ]]; then continue; fi
              echo "- [$fname]($fname)" >> "$dir/README.md"
            done
          done

      - name: Commit files
        run: |
          git config user.name "github-actions"
          git config user.email "actions@github.com"
          git add -A
          git diff --staged --quiet || git commit -m "Auto-create README index for all folders"
          git push
```

- Click **Commit changes > Commit directly to main**

### 2. Run it

- Go to top tab **Actions**
- On the left click **Auto Generate README Index**
- Click **Run workflow > Run workflow** (green button)

Wait 30 seconds, refresh your repo. Every folder will now have a `README.md` like this:

```md
# my-api-docs

Documents in this folder:

- [Architecture.md](Architecture.md)
- [Setup-Guide.md](Setup-Guide.md)
- [Database.md](Database.md)
```

And GitHub will now auto-display that when you click into the folder.

After it runs once you can delete the file `.github/workflows/auto-readme.yml` if you don't want it anymore — the READMEs will stay.

Want me to change the template to also show the first line of each doc under the link?
