# **Vercel Beginner's Guide: From Your First Website to Deployment**

>A practical, step-by-step guide for beginners · Based on Vercel's official documentation · November 2026

This guide assumes you know basic HTML, CSS, JavaScript and Git, but have never deployed a website or managed web hosting.

By the end, you should be able to:

- Explain what Vercel does and how it hosts a website.
- Deploy a website from GitHub, without manually configuring a web server.
- Publish a static website by dragging and dropping files.
- Deploy Next.js and Vite + React applications.
- Use the Vercel dashboard to manage deployments, domains and environment variables.
- Understand the basics of previews, analytics, serverless functions and storage.

**Official starting points:**

- [Vercel website](https://vercel.com)
- [Vercel documentation](https://vercel.com/docs)
- [Vercel features](https://vercel.com/features)

# **Part 1 -- What is Vercel and why use it?**

## **1.1 What is Vercel?**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/a010c657-5e31-4ddd-8974-e0280db2651d" />


[vercel.com](https://vercel.com/blog/dashboard-redesign)

**Vercel is a cloud platform for building, deploying and hosting websites and web applications.**

Think of it as a service that takes your website code and makes it accessible to people on the internet.

Normally, after writing a website, you need somewhere to host it. You might rent a server, install web server software, configure networking, manage HTTPS certificates and arrange how new versions are published.

Vercel handles much of this infrastructure for you.

A typical workflow looks like this:


<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/89af9b34-d2ad-45f6-af78-8058f39b0db0" />


**1. Your computer**

Write HTML, CSS, JavaScript or React code

**2. GitHub / GitLab**

Store your source code in a Git repository

**3. Vercel**

Build, deploy and distribute your website

**4. Your visitors**

Open your website using a public URL

For example, Vercel might publish your website at:

```
https://my-first-website.vercel.app
```

You can share this URL with anyone who has internet access, subject to your deployment's access settings.

Vercel supports frameworks including Next.js, React, Vue, Angular, Svelte and many others, as well as ordinary static HTML websites.

## **1.2 What problems does Vercel solve?**

Vercel simplifies several tasks that traditionally require server administration.

| Traditional hosting task      | What Vercel provides                         |
| ----------------------------- | -------------------------------------------- |
| Set up a web server           | Managed hosting infrastructure               |
| Configure HTTPS               | Automatic HTTPS for supported domains        |
| Upload website files manually | Automatic deployment from Git                |
| Publish updates manually      | Automatic deployments when you push changes  |
| Create a test website         | Preview deployments                          |
| Configure content delivery    | Global CDN                                   |
| Run backend code              | Vercel Functions                             |
| Manage deployment history     | Deployment history and rollback tools        |
| Configure databases           | Storage integrations through the Marketplace |

Vercel's CDN is a globally distributed network that caches and routes content close to visitors. It is automatically included with deployments.&#x20;

[Vercel CDN overview](https://vercel.com/docs/cdn)

[How Vercel CDN Works](https://vercel.com/docs/how-vercel-cdn-works)


## **1.3 Vercel compared with traditional hosting**

| Item            | Traditional shared hosting / VPS                                     | Vercel                                                                     |
| --------------- | -------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| Initial setup   | Often requires server configuration                                  | Import a project or upload files                                           |
| Web server      | You may manage Apache or Nginx                                       | Managed by Vercel                                                          |
| Deployment      | Often FTP, SSH or scripts                                            | Git integration, CLI or Vercel Drop                                        |
| HTTPS           | May require manual configuration                                     | Automatically provisioned for supported domains                            |
| CDN             | May need separate setup                                              | Included                                                                   |
| Backend         | Usually runs on your server                                          | Vercel Functions or external services                                      |
| Scaling         | Depends on server capacity and configuration                         | Managed infrastructure with plan limits                                    |
| Best suited for | General server hosting and applications needing server-level control | Frontend applications, static websites and supported full-stack frameworks |

Vercel is not a replacement for every type of server hosting. If you need a continuously running custom server, specialized networking or a persistent WebSocket server, you may need another hosting solution. Vercel Functions do not act as WebSocket servers.&#x20;



[Limits](https://vercel.com/docs/limits?)

[Run server-side code with Vercel Functions](https://vercel.com/docs/functions)


## **1.4 Who should use Vercel?**

| Developer / project                                        | Suitability                                     |
| ---------------------------------------------------------- | ----------------------------------------------- |
| Frontend developers                                        | Excellent                                       |
| Next.js developers                                         | Excellent                                       |
| React developers                                           | Excellent                                       |
| Static HTML/CSS/JavaScript websites                        | Excellent                                       |
| Portfolio and documentation websites                       | Excellent                                       |
| Small web applications                                     | Very good                                       |
| Full-stack applications                                    | Very good, with supported runtimes and services |
| Applications requiring a permanently running custom server | May need another platform                       |

For a beginner, Vercel is particularly useful because you can start with a simple HTML website and later move to React or Next.js without changing hosting platforms.

## **1.5 Important concepts explained simply**

### **A. Deployment**

A **deployment** is a published version of your website.

For example:

1. You create `index.html`.
2. You send the code to Vercel.
3. Vercel publishes the website.
4. Visitors can open the deployment URL.

Each time you publish a new version, Vercel creates another deployment.

### **B. Production deployment**

Production is the live version intended for your actual visitors.

For example:

```
https://my-website.vercel.app
```

When you push changes to your configured production branch, commonly `main`, Vercel can automatically publish the updated version.

### **C. Preview deployment**

A preview deployment is a temporary or alternative published version for testing before making changes live.

Imagine your website is already working. You want to change its layout.

Instead of changing the live website immediately:

1. Create a separate Git branch.
2. Modify your website.
3. Push the branch to GitHub.
4. Vercel creates a preview deployment.
5. Open its unique URL and test it.
6. Merge the changes into `main` when satisfied.

This lets you test changes without immediately affecting the production website.

Vercel can create preview deployments for Git branch pushes and pull requests.&#x20;

[Environments](https://vercel.com/docs/deployments/environments)

[Deploying Git Repositories with Vercel](https://vercel.com/docs/git)



### **D. Git integration**

Git integration connects your GitHub, GitLab or Bitbucket repository to Vercel.

Once connected:

- Push code to a branch and Vercel can deploy it.
- Create a pull request and Vercel can generate a preview.
- Merge into your production branch and Vercel can publish the production deployment.

You do not have to manually upload files every time you make a change.&#x20;

[Deploying Git Repositories with Vercel](https://vercel.com/docs/git)

[Builds](https://vercel.com/docs/builds)



### **E. Edge Network**

An **Edge Network** is a geographically distributed network of servers that helps deliver website content quickly.

For example, a visitor in Singapore does not necessarily need to retrieve every static file directly from a server in the United States. Vercel's network can serve cached content from a nearby location.

Vercel automatically routes deployment traffic through its CDN.&#x20;


[Vercel CDN overview](https://vercel.com/docs/cdn)

[Global network and regions](https://vercel.com/docs/regions)

### **F. Serverless Functions**

A Serverless Function is backend code that runs when requested, without you managing the underlying server.

For example, your website might contain:

```
https://my-website.vercel.app/api/temperature
```

When a visitor requests that URL, Vercel runs the associated backend function and returns a response.

You write the function; Vercel manages the infrastructure that executes it.

### **G. Edge Functions**

Edge Functions run code closer to the visitor, helping reduce response latency for suitable tasks.

They are useful for request handling, redirects, lightweight personalization and similar operations. They have different runtime capabilities and limits from Node.js-based Vercel Functions.&#x20;

[Edge Functions (Deprecated)](https://vercel.com/docs/functions/runtimes/edge/edge-functions.rsc?)

[Run server-side code with Vercel Functions](https://vercel.com/docs/functions?)



# **Part 2 -- Quick tour of Vercel's features**

## **2.1 Dashboard and Projects overview**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/726d6e7a-e95a-4cc9-9d68-239ce8719ff0" />


[supabase.com](https://supabase.com/partners/integrations/vercel?)

The dashboard is your main control centre.

Open [Vercel Dashboard](https://vercel.com/dashboard) after signing in.

A **Project** represents a website or application deployed on Vercel.

A project can contain:

- One production deployment.
- Multiple preview deployments.
- Custom domains.
- Environment variables.
- Analytics and performance information.
- Project-specific settings.

**Where to find it:** Dashboard -> select a project.

**Beginner example:** Create a project called `my-first-website` and use its dashboard to monitor your first deployment.&#x20;

[Projects overview](https://vercel.com/docs/projects?)



## **2.2 Deployments list, build logs and rollback**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/9e34097d-65fd-4e82-a5c7-aa9fe3d3ae90" />


[vercel.com](https://vercel.com/docs/projects/project-dashboard)

The Deployments section shows the history of your published website versions.

Typical deployment statuses include:

- Building: Vercel is preparing your website.
- Ready: Deployment completed successfully.
- Error: Deployment failed.
- Canceled: Deployment was stopped.

**Where to find it:** Project -> Deployments.

Select a deployment to inspect its details and build logs.

Build logs show what happened during the build, including:

- Dependency installation.
- Framework detection.
- Build commands.
- Build errors.
- Deployment completion.

### **What is rollback?**

Rollback means returning the production website to a previous deployment.

For example, suppose you publish version 2.0 but discover that a page is broken. You can restore an earlier production deployment rather than rebuilding the entire website.

On Hobby, Instant Rollback is limited to the immediately previous production deployment. Pro and Enterprise offer broader rollback choices.&#x20;

[Performing an Instant Rollback on a Deployment](https://vercel.com/docs/instant-rollback?)

[Managing Deployments](https://vercel.com/docs/deployments/managing-deployments)



**Beginner example:** You accidentally delete an important section of your website. Use rollback to restore the previous working production version.

## **2.3 Preview Deployments**

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/e38eebb5-2821-43c5-b3a9-c08a7d1df7bc" />


[maily.so](https://maily.so/danduck/posts/8do7k730rgq)

Preview deployments let you test a website before publishing changes to production.

Typical workflow:

1. Your production branch is `main`.
2. Create a branch named `new-homepage`.
3. Make changes.
4. Push the branch to GitHub.
5. Vercel creates a preview deployment.
6. Share the preview URL with someone for testing.
7. Merge the pull request when approved.

Example URL:

```
https://my-website-git-new-homepage-example.vercel.app
```

The exact URL is generated by Vercel.

**Where to find it:** Project â†’ Deployments â†’ select the Preview deployment.

**How to share it:** Open the deployment and use its Visit URL or copy the deployment URL.

**Beginner example:** Ask a colleague to check your new homepage before it goes live.&#x20;

[Environments](https://vercel.com/docs/deployments/environments?)

[Deploying Git Repositories with Vercel](https://vercel.com/docs/git)


## **2.4 Domains: custom domain, DNS and HTTPS**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/18bcbfbb-24f7-4275-993c-03f5572d7595" />


[sociumweb.com](https://www.sociumweb.com/blog/vercel/configurer-domaine-vercel)

Every deployment receives a Vercel URL, normally ending in:

```
.vercel.app
```

You can also use your own domain, such as:

```
www.example.com
```

**Where to find it:** Project -> Settings -> Domains.

Basic steps:

1. Purchase or use an existing domain.
2. Add the domain in Vercel.
3. Follow Vercel's instructions for the required DNS records.
4. Update DNS at your domain registrar if necessary.
5. Wait for DNS changes to propagate.
6. Verify that the domain is configured correctly.

DNS stands for Domain Name System. It connects a domain name to the service hosting your website.

HTTPS encrypts the connection between the browser and your website. Vercel manages HTTPS certificates for correctly configured domains.

**Beginner example:** Replace `my-website.vercel.app` with your own domain.

Hobby currently allows up to 50 custom domains per project.&#x20;

[Adding & Configuring a Custom Domain](https://vercel.com/docs/domains/working-with-domains/add-a-domain)

[Deploying & Redirecting Domains](https://vercel.com/docs/domains/working-with-domains/deploying-and-redirecting)

[Vercel Hobby Plan](https://vercel.com/docs/plans/hobby?)


## **2.5 Environment Variables**

Environment variables store configuration values separately from your source code.

For example:

```
API_KEY=your-secret-key
DATABASE_URL=your-database-connection
```

This is useful because you should not normally put passwords, API keys or database credentials directly into JavaScript files or Git repositories.

**Where to find it:** Project -> Settings -> Environment Variables.

Vercel provides three standard environments:

| Environment | Purpose           | Example                    |
| ----------- | ----------------- | -------------------------- |
| Production  | Live website      | Live API key               |
| Preview     | Test deployments  | Test API key               |
| Development | Local development | Local database credentials |

You can assign different values to each environment.

**Beginner example:** Use a test API key for preview deployments and a production API key for your live website.

Important: after changing an environment variable, redeploy the project for the new value to take effect. Also, do not expose secrets through frontend variables such as `NEXT_PUBLIC_` or `VITE_` variables; those are included in browser-delivered code.&#x20;

[Environment variables](https://vercel.com/docs/environment-variables)

[Settings](https://vercel.com/academy/vercel-foundations/vercel-settings)



## **2.6 Integrations and Marketplace**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/04898f68-dc9f-494b-b27d-aceb89906876" />

[vercel.com](https://vercel.com/changelog/reworked-integrations-and-integrations-marketplace)

The Marketplace connects Vercel with other services.

Examples include:

- Database providers.
- Monitoring and analytics tools.
- Content management systems.
- AI services.
- Testing and development tools.

There are two broad integration types:

1. **Native integrations:** Install and provision a third-party product through Vercel.
2. **Connectable accounts:** Connect an existing account from another service.

**Where to find it:** Dashboard â†’ Integrations â†’ Browse Marketplace.

**Beginner example:** Connect a database provider when your website needs to store user registrations.

Marketplace availability does not mean every integration is free. Check the provider's pricing before installing a paid product.&#x20;

[Install an Integration](https://vercel.com/docs/integrations/install-an-integration)

[Vercel Integrations](https://vercel.com/docs/integrations)



## **2.7 Analytics and Speed Insights**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/a4eabe48-0eda-48d7-bb09-42c6d7f61189" />

[vercel.com](https://vercel.com/docs/projects/project-dashboard)

Vercel provides two useful monitoring features.

**Web Analytics** helps you understand website traffic, such as page views and visitor activity.

**Speed Insights** measures real-world website performance using Core Web Vitals and related performance metrics.

Core Web Vitals are Google's user-experience measurements, including how quickly the main page content loads and how responsive the page feels.

**Where to find them:** Project â†’ Analytics or Speed Insights.

**Beginner example:** Check whether visitors are accessing your homepage and whether the page loads slowly on mobile devices.

Hobby allowances include:

- Web Analytics: 50,000 events per month across the team.
- Speed Insights: 10,000 data points and one project.

Speed Insights is available on all plans, but its included usage and paid usage differ.&#x20;

[Pricing for Web Analytics](https://vercel.com/docs/analytics/limits-and-pricing?)

[Limits and Pricing for Speed Insights](https://vercel.com/docs/speed-insights/limits-and-pricing?)

[Speed Insights Overview](https://vercel.com/docs/speed-insights?)


## **2.8 Edge Functions, Serverless Functions and Cron Jobs**

These features let you do more than serve static files.

| Feature              | What it does                              | Beginner example                 |
| -------------------- | ----------------------------------------- | -------------------------------- |
| Serverless Functions | Runs backend code in response to requests | Process a contact form           |
| Edge Functions       | Runs suitable code close to visitors      | Redirect users based on location |
| Cron Jobs            | Runs a function on a scheduled timetable  | Generate a daily report          |

**Where to find them:** Project dashboard â†’ Functions or relevant deployment/runtime settings. Cron Jobs are configured through your project code and `vercel.json`.

A Cron Job is a scheduled task. For example, you could configure a function to run every day to retrieve data or generate a report.

Vercel currently supports up to 100 Cron Jobs per project on all plans, subject to the relevant function and usage limits.&#x20;

[Run server-side code with Vercel Functions](https://vercel.com/docs/functions)

[Edge Functions (Deprecated)](https://vercel.com/docs/functions/runtimes/edge/edge-functions.rsc?)

[Cron jobs now support 100 per project on every plan](https://vercel.com/changelog/cron-jobs-now-support-100-per-project-on-every-plan)

[Limits](https://vercel.com/docs/limits?)



## **2.9 Storage: Postgres, KV, Blob and Edge Config**

Storage is where your application keeps data beyond the lifetime of a single request.

| Storage type | What it is for                                    | Example                                |
| ------------ | ------------------------------------------------- | -------------------------------------- |
| Postgres     | Relational database with tables and relationships | Store customer records                 |
| KV           | Key-value storage, often backed by Redis          | Store sessions and cached values       |
| Blob         | Store files and large objects                     | Store images, PDFs and uploads         |
| Edge Config  | Globally distributed configuration data           | Store feature flags and redirect rules |

Important distinction:

- Vercel Blob and Edge Config are Vercel storage products.
- Postgres and KV are commonly supplied through third-party Marketplace providers such as Neon, Supabase and Upstash.

Marketplace integrations can provision these resources and provide connection details through environment variables. Their pricing depends on the provider and selected plan.&#x20;

[Store application and agent data on Vercel](https://vercel.com/docs/storage)

[Storage on Vercel Marketplace](https://vercel.com/docs/marketplace-storage)



**Where to find them:** Dashboard â†’ Storage or Integrations / Marketplace, depending on the product.

**Beginner example:** Use Postgres to store website registrations and Blob to store uploaded profile photographs.

## **2.10 Team collaboration, comments and access control**


<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/9d722e5d-b5d7-43c0-8de5-fa3debf36227" />


[vercel.com](https://vercel.com/customers/delivering-ai-analysis-faster-with-the-vercel-workflow?utm_source=chatgpt.com)

Vercel provides collaboration tools that help developers review changes.

The Vercel Toolbar can provide:

- Comments pinned to parts of a preview website.
- Feature flag inspection and overrides.
- Layout shift inspection.
- Accessibility checks.
- Other development and review tools.

**Where to find it:** Open a deployment using the Toolbar-enabled Visit option.

For example, a reviewer can open a preview deployment, click a section of the page and leave a comment such as:

> Please increase the font size of this heading.

Comments are available on all plans. Team members can comment on previews by default; inviting external participants to view and comment is available on Pro and Enterprise.&#x20;

[Comments Overview](https://vercel.com/docs/comments)

[Vercel Toolbar](https://vercel.com/docs/vercel-toolbar)


For access control, Vercel also provides team roles and permissions. Advanced team collaboration and role-based access controls depend on the plan.&#x20;

[Vercel Hobby Plan](https://vercel.com/docs/plans/hobby?)

[Scale your app, control your costs](https://vercel.com/pricing)

[Limits](https://vercel.com/docs/limits)

## **2.11 Vercel pricing: what can beginners use for free?**

Vercel's Hobby plan is free and intended for personal, non-commercial projects.

Some important Hobby allowances documented by Vercel include:

| Resource                          | Hobby allowance     |
| --------------------------------- | ------------------- |
| Projects                          | 200                 |
| Deployments per day               | 100                 |
| Concurrent builds                 | 1                   |
| Build execution                   | 6,000 minutes       |
| Maximum build time per deployment | 45 minutes          |
| Static file upload size           | 100 MB              |
| Function invocations              | 1 million           |
| Function duration                 | 100 GB-hours        |
| Web Analytics                     | 50,000 events/month |
| Speed Insights                    | 10,000 data points  |
| Speed Insights projects           | 1                   |
| Edge Config reads                 | 100,000             |
| Edge Config writes                | 100                 |

These are usage allowances, not guarantees that every feature or third-party integration is free. If you exceed Hobby usage limits, access to the relevant resource may be paused until the allowance resets.&#x20;





For professional and commercial projects, review the Pro plan and its usage-based charges.

Official pricing: [Vercel pricing](https://vercel.com/pricing).

# **Part 3 -- Step-by-step setup instructions**

This section uses GitHub as the example. The same general workflow applies to GitLab and Bitbucket.

## **3.1 Before you begin**

You need:

- A computer with internet access.
- A GitHub, GitLab or Bitbucket account.
- A website project containing HTML, CSS and JavaScript, or a supported framework project.
- A Git repository containing your project, for the Git-based workflow.

You do not need to rent a server or configure a web server.

## **3.2 Create a Vercel account**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/b342f949-b50e-4914-ba23-2d8676d162b1" />


[note.com](https://note.com/oto_wan_ai/n/nee63af74641b)

1. Open [Vercel Sign Up](https://vercel.com/signup).
2. Choose GitHub, GitLab or Bitbucket as your sign-in method.
3. Authorize Vercel when your Git provider asks for permission.
4. Complete the account setup.
5. Select the Hobby plan if you are deploying personal, non-commercial projects.

**What you should see:** Your Vercel dashboard.

**If it fails:**

- Confirm that you can sign in to your Git provider.
- Check whether your browser blocks pop-ups or third-party sign-in.
- Try signing in again using the same provider.

## **3.3 Connect your Git provider and grant permissions**

When you sign up using GitHub, Vercel asks to connect to your GitHub account.

1. Open your Vercel dashboard.
2. Look for the option to add a new project.
3. Choose Import Git Repository.
4. If necessary, select Configure GitHub App or install the Vercel GitHub integration.
5. Authorize access to the repositories you want Vercel to use.

You can normally choose whether to grant access to all repositories or selected repositories, depending on the permissions offered during installation.

**What you should see:** A list of repositories that Vercel can access.

**If your repository is missing:**

- Check that you selected the correct GitHub account or organization.
- Check the GitHub App's repository access permissions.
- If it belongs to an organization, ask an organization administrator to approve or install the application if required.

Vercel's Git integration supports GitHub, GitLab, Bitbucket and Azure DevOps Pipelines.&#x20;

[Deploying Git Repositories with Vercel](https://vercel.com/docs/git)



## **3.4 Import your first project from Git**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/430640e0-561b-4d22-ac60-517a2a20d7e0" />


[blog.serverworks.co.jp](https://blog.serverworks.co.jp/2024/07/03/152259)

1. In Vercel, click **Add New -> Project**.
2. Locate your Git repository.
3. Click **Import**.
4. Enter a project name.
5. Review the framework and build settings.
6. Configure environment variables if your application needs them.

**What you should see:** A project configuration screen with a Deploy button.

**If your repository does not appear:** Return to the Git provider permissions from the previous step.

## **3.5 Understand the import settings**

This is one of the most important steps.

Vercel tries to detect your framework and configure the build automatically. You should normally accept the detected defaults unless you have a specific reason to change them.

| Setting          | Meaning                                       | Beginner guidance                            |
| ---------------- | --------------------------------------------- | -------------------------------------------- |
| Framework Preset | Identifies your framework                     | Select Next.js or Vite if detected correctly |
| Root Directory   | Folder containing the application             | Usually `./`                                 |
| Build Command    | Command that creates the production build     | Usually automatic                            |
| Output Directory | Folder containing the generated website files | Usually automatic                            |
| Install Command  | Command that installs dependencies            | Usually automatic                            |

### **What does each setting mean?**

**Framework Preset**

This tells Vercel how your application is built.

Examples:

- Next.js
- Vite
- React
- Other

**Root Directory**

This is the folder where your website project is located.

For example:

```
my-project/
├── README.md
└── website/
    ├── package.json
    ├── index.html
    └── src/
```

If your application is inside `website`, the Root Directory should be:

```
website
```

**Build Command**

This is the command Vercel executes to create your production website.

For example:

```
npm run build
```

**Output Directory**

This is where your build tool puts the generated files.

Typical examples:

```
Vite       -> dist
Create React App -> build
Next.js    -> .next (framework-managed)
```

For Next.js, normally leave the output directory at its automatic framework setting.

**Install Command**

This installs the packages your project needs.

For example:

```
npm install
```

Vercel automatically detects common framework settings and install commands.&#x20;

[Configuring a Build](https://vercel.com/docs/builds/configure-a-build?)

[Builds](https://vercel.com/docs/builds?)


## **3.6 Configure environment variables**

If your project uses an API key, database connection or other configuration, add it before deploying.

Example:

```
API_URL=https://api.example.com
API_KEY=your-secret-key
```

In the Vercel project configuration screen:

1. Find Environment Variables.
2. Enter the variable name.
3. Enter its value.
4. Select the environments where it should apply.
5. Click Add or Save.

For a beginner, you can usually select Production, Preview and Development if the same value is appropriate for all three.

**Important security reminder:** Never commit passwords, API keys or private credentials to GitHub. Also, variables exposed to browser-side code are not secret.

**What you should see:** Your environment variables listed in the project configuration.

**If a variable is missing later:** Check the variable name, selected environment and whether you redeployed after adding it.&#x20;

[Environment variables](https://vercel.com/docs/environment-variables)

[Settings](https://vercel.com/academy/vercel-foundations/vercel-settings)

## **3.7 Click Deploy**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/101ffc5f-5f4a-4672-afd1-0f9e7eabec45" />


[vercel.com](https://vercel.com/product-tour?)

Click **Deploy**.

Vercel will perform a series of operations:

1. **Retrieve your source code**

   Vercel downloads the selected Git repository and branch.
2. **Install dependencies**

   It installs the packages listed in your project's package configuration.
3. **Build the application**

   It executes the configured build command, if one is required.
4. **Prepare the deployment**

   Vercel prepares the generated website files and any supported functions.
5. **Publish the deployment**

   Vercel makes the deployment available through a generated URL.

**What you should see:** A deployment progress screen followed by a successful deployment confirmation.

If the build fails, open the build logs. The first meaningful error is usually more useful than the final generic failure message.

Common causes include:

- Missing dependencies.
- Incorrect build command.
- Incorrect root directory.
- JavaScript or TypeScript errors.
- Missing environment variables.

## **3.8 Open the live URL**

When the deployment succeeds:

1. Click **Visit**.
2. Your website opens in a new browser tab.
3. Test your homepage.
4. Test images, CSS, JavaScript and navigation.
5. Check the website on both desktop and mobile.

Your first deployment URL will resemble:

```
https://your-project-name.vercel.app
```

**What you should see:** Your website running on a public URL.

## **3.9 Make a change and trigger automatic redeployment**

This is where Git integration becomes particularly useful.

Suppose you want to change your homepage heading.

Edit your file:

```
<h1>My First Vercel Website</h1>
```

Change it to:

```
<h1>Welcome to My Website!</h1>
```

Save the file and execute:

```
git add .
git commit -m "Update homepage heading"
git push origin main
```

Vercel detects the new commit and automatically starts another deployment.

**What you should see:**

1. A new deployment appears in the Vercel Deployments list.
2. Its build status changes as Vercel processes it.
3. When it is Ready, your production website reflects the change.

The Git integration supports automatic deployments for branch pushes and production branch updates.&#x20;

[Deploying Git Repositories with Vercel](https://vercel.com/docs/git)

[Builds](https://vercel.com/docs/builds)



## **3.10 First-deployment setup checklist**

Use this checklist to verify that everything is ready.

- Vercel account created.
- GitHub, GitLab or Bitbucket connected.
- Correct repository selected.
- Framework detected correctly.
- Root directory verified.
- Build and output settings checked.
- Environment variables configured if needed.
- Deployment completed successfully.
- Website opens using its Vercel URL.
- Images, CSS, JavaScript and navigation work.
- A test Git commit triggers another deployment.

# **Part 4 -- Tutorials: Deploy four different projects**

The four tutorials below cover different ways to publish a website.

| Tutorial | Project type          | Deployment method | Git required?               |
| -------- | --------------------- | ----------------- | --------------------------- |
| A        | Next.js               | GitHub import     | Yes                         |
| B        | Static HTML/CSS/JS    | Vercel Drop       | No                          |
| C        | Vite + React          | GitHub import     | Yes                         |
| D        | Any supported project | Vercel CLI        | No Git integration required |

## **Tutorial A -- Deploy a Next.js application from GitHub**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/586c8d4f-83f6-47ea-a49d-d83e434103cb" />


[vercel.com](https://vercel.com/docs/projects/project-dashboard?)

Next.js is a React framework used to build websites and full-stack applications.

### **Step 1: Create a Next.js application**

You need Node.js and npm installed.

Open PowerShell, Terminal or the VS Code terminal.

Run:

```
npx create-next-app@latest my-nextjs-site
```

The setup tool will ask questions about the project.

For a beginner, accept the recommended defaults. If prompted, TypeScript, ESLint, Tailwind CSS and the App Router are reasonable starting choices.

Move into the project folder:

```
cd my-nextjs-site
```

### **Step 2: Start the application locally**

Run:

```
npm run dev
```

Expected terminal output will resemble:

```
> my-nextjs-site@0.1.0 dev
> next dev

Local: http://localhost:3000
```

Open:

```
http://localhost:3000
```

You should see the default Next.js starter page.

Stop the development server with:

```
Ctrl + C
```

### **Step 3: Understand the folder structure**

A typical generated project looks like this:

```
my-nextjs-site/
├── app/
│   ├── favicon.ico
│   ├── globals.css
│   ├── layout.tsx
│   └── page.tsx
├── public/
├── node_modules/
├── package.json
├── package-lock.json
├── next.config.ts
├── tsconfig.json
└── README.md
```

Important files:

| File/folder       | Purpose                  |
| ----------------- | ------------------------ |
| `app/page.tsx`    | Homepage                 |
| `app/layout.tsx`  | Shared page layout       |
| `app/globals.css` | Global CSS               |
| `public/`         | Static images and assets |
| `package.json`    | Dependencies and scripts |
| `next.config.ts`  | Next.js configuration    |

### **Step 4: Create a GitHub repository**

1. Open [GitHub: Create a new repository](https://github.com/new).
2. Name it `my-nextjs-site`.
3. Choose Public or Private as appropriate.
4. Do not initialize it with an additional README if your local project already has one.
5. Click Create repository.

### **Step 5: Push the project to GitHub**

In your project folder:

```
git init
git add .
git commit -m "Initial Next.js website"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/my-nextjs-site.git
git push -u origin main
```

Replace `YOUR_USERNAME` with your GitHub username.

Your folder is now stored in GitHub.

### **Step 6: Import the project into Vercel**

1. Open [Vercel New Project](https://vercel.com/new).
2. Sign in with GitHub if necessary.
3. Find `my-nextjs-site`.
4. Click Import.
5. Check the Framework Preset.

Expected settings:

| Setting          | Expected value |
| ---------------- | -------------- |
| Framework Preset | Next.js        |
| Root Directory   | `./`           |
| Build Command    | Automatic      |
| Output Directory | Automatic      |
| Install Command  | Automatic      |

6\. Click Deploy.

### **Step 7: Verify the deployment**

Wait for the build to finish.

Click Visit.

Your website should be accessible using a URL resembling:

```
https://my-nextjs-site.vercel.app
```

Tutorial A complete

You have successfully deployed your first Next.js website.

## **Tutorial B -- Deploy a static HTML, CSS and JavaScript website without Git**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/c7d788be-7f44-4c74-b633-39f8775d29c8" />

[vercel.com](https://vercel.com/changelog/vercel-drop)

Vercel Drop allows you to deploy a file, folder or ZIP archive by dragging it into your browser.

This is ideal for simple websites that do not need Git integration.

Official entry point: [Vercel Drop](https://vercel.com/drop).&#x20;

[4 ways to put a website on Vercel without Git or a CLI](https://vercel.com/i/deploy-to-vercel-without-git)

[Introducing Vercel Drop](https://vercel.com/changelog/vercel-drop?)

### **Step 1: Create your website folder**

Create a folder named:

```
my-static-site/
```

Inside it, create:

```
my-static-site/
├── index.html
├── style.css
└── script.js
```

### **Step 2: Create index.html**

```
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>My Static Website</title>
    <link rel="stylesheet" href="style.css">
</head>

<body>

    <h1>My First Vercel Website</h1>

    <p>This website was deployed without Git.</p>

    <button onclick="showMessage()">
        Click Me
    </button>

    <script src="script.js"></script>

</body>
</html>
```

### **Step 3: Create style.css**

```
body {
    font-family: Arial, sans-serif;
    text-align: center;
    margin-top: 100px;
    background: #f0f4f8;
}

h1 {
    color: #1565c0;
}

button {
    padding: 12px 24px;
    background: #1565c0;
    color: white;
    border: none;
    border-radius: 6px;
    cursor: pointer;
}
```

### **Step 4: Create script.js**

```
function showMessage() {
    alert("Your website is working!");
}
```

### **Step 5: Test locally**

Open `index.html` in your browser.

You should see:

- A heading.
- A paragraph.
- A button.

Click the button. An alert should appear.

### **Step 6: Deploy using Vercel Drop**

1. Open [Vercel Drop](https://vercel.com/drop).
2. Sign in to your Vercel account if prompted.
3. Drag the complete `my-static-site` folder into the browser drop area.
4. Select your Vercel team.
5. Enter a project name.
6. Click Deploy.

You can also upload a ZIP archive containing the website files.

For a static site, Vercel can serve the files without a framework build step. Make sure `index.html` is at the root of the uploaded site folder.&#x20;

[Introducing Vercel Drop](https://vercel.com/changelog/vercel-drop?)

[Builds](https://vercel.com/docs/builds)

### **Step 7: Verify**

Vercel will provide a URL resembling:

```
https://my-static-site.vercel.app
```

Open it.

Test the button again. If the alert appears, the HTML, CSS and JavaScript are all working.

Tutorial B complete

**Important:** A Vercel Drop deployment does not automatically redeploy when you edit your local files. You must upload the updated project again, or connect the project to Git for automatic deployments.

## **Tutorial C -- Deploy a Vite + React application**

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/4810f2ab-bd5c-425c-a16c-0491d83048d6" />


[apryse.com](https://apryse.com/blog/pdf-editor-using-vite-and-react?)

Vite is a frontend development and build tool. React is a JavaScript library for building user interfaces.

This tutorial creates a small React application and deploys it through GitHub.

### **Step 1: Create the project**

Run:

```
npm create vite@latest my-react-site -- --template react
```

Enter the folder:

```
cd my-react-site
```

Install dependencies:

```
npm install
```

### **Step 2: Run the application locally**

```
npm run dev
```

Expected output resembles:

```
VITE ready

Local: http://localhost:5173/
```

Open:

```
http://localhost:5173/
```

You should see the default Vite + React starter page.

Stop the server:

```
Ctrl + C
```

### **Step 3: Understand the folder structure**

```
my-react-site/
├── public/
├── src/
│   ├── assets/
│   ├── App.css
│   ├── App.jsx
│   ├── index.css
│   └── main.jsx
├── index.html
├── package.json
├── package-lock.json
└── vite.config.js
```

Important files:

| File             | Purpose                      |
| ---------------- | ---------------------------- |
| `index.html`     | Main HTML entry point        |
| `src/App.jsx`    | Main React component         |
| `src/App.css`    | Component styling            |
| `src/main.jsx`   | Starts the React application |
| `package.json`   | Dependencies and scripts     |
| `vite.config.js` | Vite configuration           |

### **Step 4: Build the application**

Run:

```
npm run build
```

Vite creates a production build in:

```
dist/
```

The `dist` folder contains the files Vercel will serve.

### **Step 5: Upload the project to GitHub**

Create a GitHub repository named:

```
my-react-site
```

Then run:

```
git init
git add .
git commit -m "Initial Vite React website"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/my-react-site.git
git push -u origin main
```

Replace `YOUR_USERNAME` with your GitHub username.

### **Step 6: Import into Vercel**

1. Open [Vercel New Project](https://vercel.com/new?).
2. Import `my-react-site`.
3. Check the build settings.

Expected settings:

| Setting          | Value           |
| ---------------- | --------------- |
| Framework Preset | Vite            |
| Root Directory   | `./`            |
| Build Command    | `npm run build` |
| Output Directory | `dist`          |
| Install Command  | `npm install`   |

Vercel normally detects these settings automatically.&#x20;

[Configuring a Build](https://vercel.com/docs/builds/configure-a-build)

[Builds](https://vercel.com/docs/builds)




4. Click Deploy.

### **Step 7: Verify the website**

After deployment, click Visit.

Your URL will resemble:

```
https://my-react-site.vercel.app
```

Confirm that the React starter page appears.

Tutorial C complete

## **Tutorial D -- Deploy using the Vercel CLI**

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/440520bc-954a-430f-b83e-1ac8e6022977" />

[vercel.com](https://vercel.com/blog/advanced-project-settings)

The Vercel CLI is a command-line tool that lets you deploy and manage projects directly from your computer.

This is useful when you want to automate deployments or work without using the dashboard for every deployment.

### **Step 1: Install Node.js**

Download and install Node.js from:

[Node.js official website](https://nodejs.org/).

Verify the installation:

```
node --version
npm --version
```

You should see version numbers for both commands.

### **Step 2: Install Vercel CLI**

Run:

```
npm i -g vercel
```

Verify:

```
vercel --version
```

You should see the installed Vercel CLI version.

Official instructions: [Vercel CLI documentation](https://vercel.com/docs/cli?).

### **Step 3: Log in**

Run:

```
vercel login
```

Follow the login prompts. Depending on the authentication flow, you may be asked to verify your email or complete browser authentication.

### **Step 4: Prepare a project**

You can use the static website created in Tutorial B.

For example:

```
my-static-site/
├── index.html
├── style.css
└── script.js
```

Open a terminal in this directory.

### **Step 5: Deploy a Preview**

Run:

```
vercel
```

The first deployment may ask you:

- Which Vercel account or team to use.
- Whether to set up and deploy the current directory.
- Whether to link to an existing project or create a new one.
- What project name to use.
- Which directory contains the project files.

For a simple static website, use the current directory as the project root and the appropriate static-site settings.

Vercel will upload and deploy your project.

Expected output resembles:

```
Vercel CLI

? Set up and deploy this project? Yes
? Which scope? Your account
? Link to existing project? No
? What's your project's name? my-static-site

Deploying...
Building...
Ready

https://my-static-site-xxxx.vercel.app
```

The exact prompts and output may vary by CLI version and project configuration.

### **Step 6: Deploy to Production**

After verifying your preview deployment, run:

```
vercel --prod
```

This creates a production deployment.

Expected output includes a production deployment URL and a Ready status.

### **Step 7: Verify**

Open the production URL printed by the CLI.

You should see your website.

The difference between these two commands is important:

| Command         | Deployment type |
| --------------- | --------------- |
| `vercel`        | Preview         |
| `vercel --prod` | Production      |

The CLI uses Preview by default unless you specify `--prod`.&#x20;

[vercel deploy](https://vercel.com/docs/cli/deploy)

[Builds](https://vercel.com/docs/builds)

[Deploying a project from the CLI](https://vercel.com/docs/projects/deploy-from-cli)

Tutorial D complete

# **Part 5 -- Next steps and troubleshooting**

## **5.1 Common deployment errors**

These are some of the problems beginners encounter when deploying their first websites.

| Error                            | Possible cause                                                        | How to fix it                                                                                           |
| -------------------------------- | --------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| **Build failed**                 | Syntax error, missing dependency or incorrect build configuration     | Open the deployment's Build Logs. Find the first actual error, fix it locally and push a new commit.    |
| **404 after deployment**         | Missing `index.html`, incorrect output directory or incorrect routing | Check that the deployed output contains the homepage. For Vite, confirm the output directory is `dist`. |
| **Environment variable missing** | Variable not configured, incorrect name or wrong environment selected | Go to Settings â†’ Environment Variables. Check the name, value and environment. Redeploy.                |
| **Domain not working**           | Incorrect DNS records or DNS propagation still in progress            | Go to Settings â†’ Domains and follow Vercel's DNS configuration instructions.                            |
| **Build command not found**      | Incorrect command or missing `package.json` script                    | Check `package.json`, confirm dependencies are installed and correct the Build Command.                 |

### **More troubleshooting examples**

**Problem 1: Build failed with&#x20;**`npm run build`

Open your local terminal and test:

```
npm install
npm run build
```

If the command fails locally, fix the problem before deploying again.

**Problem 2: Website shows 404**

For a Vite application, check:

```
my-react-site/
├── index.html
├── src/
└── dist/
    ├── index.html
    └── assets/
```

Make sure Vercel is configured to serve:

```
dist
```

For a plain HTML website, ensure `index.html` is in the root of the published directory.

**Problem 3: Environment variable is undefined**

For example, your code expects:

```
const apiKey = process.env.API_KEY;
```

But the variable may have been configured as:

```
APIKEY
```

The names do not match.

Also remember that browser-side JavaScript cannot safely access private server secrets. Use a backend function when a secret must remain private.

**Problem 4: Custom domain does not work**

Check:

1. Is the domain added to the correct Vercel project?
2. Are the DNS records correct?
3. Is the domain verified?
4. Has DNS propagation completed?
5. Is the domain assigned to the intended production deployment?

**Problem 5: npm run build is not found**

Open `package.json`.

For example:

```
{
  "scripts": {
    "dev": "vite",
    "build": "vite build",
    "preview": "vite preview"
  }
}
```

If the `build` script is missing, add the correct script for your framework.

Vercel's build configuration documentation explains how to customize framework detection, build commands and output directories.&#x20;

[Configuring a Build](https://vercel.com/docs/builds/configure-a-build)

[Builds](https://vercel.com/docs/builds?)

[Adding & Configuring a Custom Domain](https://vercel.com/docs/domains/working-with-domains/add-a-domain)


## **5.2 A 10-minute practice plan: Your first three deployments**

The objective is to experience three different ways to publish a website.

10-minute challenge

This is a practice target assuming your Vercel account, Node.js installation and sample files or repository are already prepared. First-time account setup or dependency installation can take longer.

**Deployment 1 -- Static website**

Minutes 0-3

Use Vercel Drop without Git.

1. Open [Vercel Drop](https://vercel.com/drop).
2. Drag a folder containing `index.html`.
3. Click Deploy.
4. Open the generated website URL.

**Deployment 2 -- GitHub website**

Minutes 3-7

Import an existing GitHub repository.

1. Open [New Vercel Project](https://vercel.com/new).
2. Import your GitHub repository.
3. Accept the detected settings.
4. Click Deploy.
5. Open the generated URL.

**Deployment 3 -- CLI deployment**

Minutes 7-10

Deploy a local static website using the CLI.

1. Open your project folder.
2. Run `vercel`.
3. Complete the initial project setup if prompted.
4. Open the preview URL.
5. Run `vercel --prod` to publish to production.

At the end of the exercise, you should understand three deployment approaches:

- Drag and drop.
- Git-based automatic deployment.
- Command-line deployment.

## **5.3 What should you learn next?**

Once you can deploy a basic website, I recommend learning these topics in order:

| Learning stage | Topic                         | What you will gain                              |
| -------------- | ----------------------------- | ----------------------------------------------- |
| Beginner       | Custom domains                | Use your own website address                    |
| Beginner       | Preview deployments           | Test changes before publishing                  |
| Beginner       | Environment variables         | Manage application configuration                |
| Intermediate   | Vercel Functions              | Create backend API endpoints                    |
| Intermediate   | Storage                       | Save application data                           |
| Intermediate   | Analytics                     | Understand visitors and performance             |
| Intermediate   | Cron Jobs                     | Automate scheduled tasks                        |
| Advanced       | Edge Functions                | Run request-handling code closer to visitors    |
| Advanced       | CLI and deployment automation | Integrate deployment into development workflows |

## **5.4 Official Vercel documentation and learning resources**

These are useful links to bookmark.

[image](https://www.google.com/s2/favicons?domain=https://vercel.com\&sz=32)

**Getting started**

- [Vercel official website](https://vercel.com)
- [Vercel documentation](https://vercel.com/docs)
- [Getting started with Vercel](https://vercel.com/docs/getting-started-with-vercel)
- [Vercel dashboard](https://vercel.com/dashboard)

  [image](https://www.google.com/s2/favicons?domain=https://vercel.com\&sz=32)

  **Deployment and Git**
- [Deployments documentation](https://vercel.com/docs/deployments)
- [Git integration](https://vercel.com/docs/git)
- [Production and Preview environments](https://vercel.com/docs/deployments/environments)
- [Build configuration](https://vercel.com/docs/builds/configure-a-build)
- [Vercel Drop](https://vercel.com/drop)

  [image](https://www.google.com/s2/favicons?domain=https://vercel.com\&sz=32)

  **Domains and configuration**
- [Domains documentation](https://vercel.com/docs/domains)
- [Environment variables](https://vercel.com/docs/environment-variables)
- [Project configuration](https://vercel.com/docs/project-configuration)

  [image](https://www.google.com/s2/favicons?domain=https://vercel.com\&sz=32)

  **Backend and storage**
- [Vercel Functions](https://vercel.com/docs/functions)
- [Edge runtime](https://vercel.com/docs/functions/runtimes/edge)
- [Cron Jobs](https://vercel.com/docs/cron-jobs)
- [Storage](https://vercel.com/docs/storage)
- [Marketplace](https://vercel.com/marketplace)

  [image](https://www.google.com/s2/favicons?domain=https://vercel.com\&sz=32)

  **Monitoring and pricing**
- [Web Analytics](https://vercel.com/docs/analytics)
- [Speed Insights](https://vercel.com/docs/speed-insights)
- [Platform limits](https://vercel.com/docs/limits)
- [Pricing](https://vercel.com/pricing)

  [image](https://www.google.com/s2/favicons?domain=https://vercel.com\&sz=32)

  **Command-line tools**
- [Vercel CLI](https://vercel.com/docs/cli)
- [Deploy using CLI](https://vercel.com/docs/projects/deploy-from-cli)

## **Final recommendation**

If you are completely new to Vercel, learn it in this order:

1. **Start with static HTML deployment.** Understand how files become a live website.
2. **Connect GitHub.** Learn automatic deployments and preview URLs.
3. **Try Next.js or Vite + React.** Understand how a build process works.
4. **Learn environment variables and domains.**
5. **Explore Functions, Storage and Analytics.**

You do not need to learn all of Vercel before deploying your first website. Start with a simple HTML project, publish it, make one change and deploy again. That will teach you the most important part of the platform: the deployment workflow.
