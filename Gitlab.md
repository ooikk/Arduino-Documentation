# GitLab Beginner Guide: From Your First Repository to CI/CD and Website Deployment

A practical, step-by-step guide for beginners · Based on official GitLab documentation · November 2026

[GitLab Announces the General Availability of GitLab Duo](https://images.openai.com/static-rsc-4/EoxXf6ChMqDLbbcNuU68IT7w-8u40o6oM1l6JMr57DWSKruIfv6ZYVqR-35Bq8A3viqXxCBW8bjD_FQeyhmdgo0ZCVdHdGkhoFsGVZf0rHy8TkW3kJaJqatC5Et0t6_CAOlbriclgeBguq7OuJ3fIqo4fSZmJ8SqD6-1C9CLBnI?purpose=inline)

[globenewswire.com](https://www.globenewswire.com/dc/news-release/2024/04/18/2865808/0/en/GitLab-Announces-the-General-Availability-of-GitLab-Duo-Chat.html?utm_source=chatgpt.com)

## Introduction

This guide teaches you how to use **GitLab from the beginning**, assuming you already understand basic Git commands such as `git init`, `git add`, `git commit`, `git push`, and `git pull`.

You do not need previous experience with:

- GitLab or other Git hosting platforms.
- Software project management.
- DevOps (Development and Operations).
- CI/CD (Continuous Integration and Continuous Delivery).
- Automated software testing or deployment.

By the end of this guide, you will be able to:

1. Create a GitLab account, group, and project.
2. Upload and manage source code.
3. Organise development work using Issues and Merge Requests.
4. Collaborate with other developers.
5. Automate code checking and testing using CI/CD.
6. Deploy a static website using GitLab Pages.
7. Understand GitLab's security and DevSecOps capabilities.

**The examples use GitLab.com**, the cloud-hosted version of GitLab. The instructions are intended for Windows users, but the Git commands also work on macOS and Linux unless otherwise indicated.

Official websites:

- [GitLab main website](https://about.gitlab.com/?utm_source=chatgpt.com)
- [GitLab documentation](https://docs.gitlab.com/?utm_source=chatgpt.com)
- [GitLab platform overview](https://about.gitlab.com/platform/?utm_source=chatgpt.com)
- [GitLab.com](https://gitlab.com/?utm_source=chatgpt.com)

# **PART 1 — WHAT IS GITLAB AND WHY USE IT?**

## **1.1 What is GitLab?**

GitLab is a platform for managing software development projects.

At its simplest, GitLab provides an online location to store your source code using Git.

However, it does much more than store code.

It combines several software development activities into one platform:

- Source code management.
- Project planning and task tracking.
- Code review and collaboration.
- Automated building and testing.
- Software deployment.
- Security scanning.
- Software package and container management.

For example, imagine you are developing an ESP32-S3 IoT project.

Without GitLab, you might use:

- A local computer to write Arduino code.
- Google Drive to share project documents.
- Email to discuss bugs.
- Another service to run automated tests.
- A separate server to distribute firmware.

With GitLab, you can organise these activities within one project.

[GitLab's Security Officer on an Easier Path to App Security - The New Stack](https://images.openai.com/static-rsc-4/TDyD0Sw2bDfhPpDYAtpbINDPJ6YrLZi56TFzAz4d6So24txZV8nW1ItlcrmNAD6aih7lNmdyppdlrgmQu0uCjXfa2-NDjJ3XeHBYlP2dQ_3i6RqqWna50gsDYFYU3BbeWhyEm-ugPtsJtoACk2ftOb-rLX6AMciHv7RjwYckTeA?purpose=inline)

[thenewstack.io](https://thenewstack.io/gitlabs-security-officer-on-an-easier-path-to-app-security/?utm_source=chatgpt.com)

GitLab describes this approach as an integrated DevSecOps platform: development, operations, and security activities are connected throughout the software lifecycle.

## **1.2 Git vs GitHub vs GitLab**

It is important to understand that Git, GitHub, and GitLab are not the same thing.

| Item   | What it is                       | Main purpose                                                                |
| ------ | -------------------------------- | --------------------------------------------------------------------------- |
| Git    | Version control software         | Tracks changes to source code                                               |
| GitHub | Cloud-based development platform | Hosts Git repositories and collaboration tools                              |
| GitLab | DevSecOps platform               | Hosts repositories and integrates planning, CI/CD, security, and deployment |

**Git** is the underlying version control system. It works on your computer even without an internet connection.

**GitHub** and **GitLab** provide online services built around Git.

[GitHub常用指令 | lhldudu's blog](https://images.openai.com/static-rsc-4/ErI3L4O8SJCOb0vigkdXGsIsjI_GAtx9KU9MVpKmIPrmO-4HY-grsYnOnHWAMVy3AoHsWyGySqDHPlhbb8_jn3R3-dVWGxOzTAHgAwdYKWyl5kx35YF4IZfnlk7hS4JCTYU0tU-DhFUnvaJB71mkhCDnuGRC4EqEtGQHGLCTmEc?purpose=inline)

**GitHub**

Popular for open-source collaboration, repositories, GitHub Actions, and its developer ecosystem.

[Official website](https://github.com/?utm_source=chatgpt.com)

[GitLab 19.2 Brings Governed Agentic Automation to Clear the Backlog AI Coding Creates](https://images.openai.com/static-rsc-4/spFPvtQoz6H4Kv_JPvFvsnUQx561jFO2rC8TeQ5z74A7L7JnIA_QZiok5eSfBFGnp8NU5LHFQrr4-Yci_mClLuagSBq7gO2k58fRPAKbifMp5pbTnaOctJm7h63yd_uFxd-S6iNCNMa5OJUZDcSngbbQRZv3Awp9RtJaWpRDKXY?purpose=inline)

**GitLab**

Provides integrated project planning, source control, CI/CD, security, and deployment capabilities.

[Official website](https://gitlab.com/?utm_source=chatgpt.com)

### **Why choose GitLab?**

GitLab is particularly attractive when you want to connect the entire development workflow.

For example:

1. Create an Issue describing a new ESP32 sensor feature.
2. Create a development branch.
3. Write the code.
4. Push your changes to GitLab.
5. GitLab automatically runs tests.
6. Open a Merge Request for review.
7. Merge the approved changes.
8. Automatically build or deploy the application.

This workflow can be managed within GitLab without having to combine multiple unrelated services.

GitLab and GitHub are both capable platforms. Neither is universally better; the choice depends on your development workflow, integrations, hosting requirements, and subscription needs.

## **1.3 Who should use GitLab?**

| User                   | How GitLab helps                      | Example                                |
| ---------------------- | ------------------------------------- | -------------------------------------- |
| Solo developer         | Stores code and automates testing     | Manage personal ESP32 projects         |
| Small development team | Coordinates tasks and reviews changes | Develop an IoT application together    |
| Software company       | Manages development workflows         | Control releases across multiple teams |
| DevOps engineer        | Automates building and deployment     | Deploy applications automatically      |
| Security team          | Integrates security scanning          | Identify vulnerable dependencies       |
| Educator               | Organises student projects            | Manage programming assignments         |

For learning and personal development, GitLab Free provides a useful starting point.

## **1.4 Important GitLab concepts**

Before using GitLab, understand these terms.

| Term               | Simple explanation                                                            | Example                            |
| ------------------ | ----------------------------------------------------------------------------- | ---------------------------------- |
| Repository (repo)  | A place where source code and its change history are stored                   | ESP32 firmware source code         |
| Group              | A container for organising related projects and users                         | ESP32-Course                       |
| Project            | A workspace containing a repository and associated development tools          | ESP32-Sensor                       |
| Namespace          | The account or group name under which a project is organised                  | `ooikk/ESP32-Sensor`               |
| Issue              | A task, bug, or feature request                                               | Add temperature sensor support     |
| Branch             | An independent line of development                                            | `feature/temperature-sensor`       |
| Commit             | A saved set of changes to the repository                                      | Add sensor reading function        |
| Merge Request (MR) | A request to incorporate changes from one branch into another                 | Merge a sensor feature into `main` |
| Pipeline           | An automated sequence of development tasks                                    | Build, test, deploy                |
| Job                | One task inside a pipeline                                                    | Run unit tests                     |
| Runner             | The computer or execution environment that runs CI/CD jobs                    | GitLab-hosted Linux machine        |
| Artifact           | A file produced by a job and saved for later use                              | Compiled firmware binary           |
| DevSecOps          | Integrating development, operations, and security into the software lifecycle | Test, scan, and deploy software    |

### **Group vs Project vs Repository**

These three concepts are frequently confused.

[Overview of GitLab Group and Project Structure | Courseware as Code Workshop](https://images.openai.com/static-rsc-4/HBT9A0TRU9cJvv8AOW6BvarW7S5NJXrTogb-uFwCaXSCF3GvxBMETqxszUJkTBVV5_F2I9iwUr-f4yZ64YWLb_VQEaEeAmh08YULQSVAqVDObDFERFeWWXhNd_dyBFjJQKBHXFdF2HrRHF1SvLcymCvfOGrPAjg7vR0r2qZafzo?purpose=inline)

[devops-education.gitlab.io](https://devops-education.gitlab.io/cwac-workshop/course/gitlab_overview/?utm_source=chatgpt.com)

Think of the structure as follows:

```
GitLab Account
│
└── Group: ESP32-Course
    │
    ├── Project: ESP32-Basics
    │   └── Repository
    │
    ├── Project: ESP32-WiFi
    │   └── Repository
    │
    └── Project: ESP32-OTA
        └── Repository
```

- The **Group** organises related projects.
- The **Project** is the main workspace.
- The **Repository** stores the actual source code and Git history.

A GitLab project also contains Issues, Merge Requests, CI/CD settings, and other project features.

A namespace is the URL and organisational identity under which the project exists. It can be a personal namespace or a group namespace.

Official documentation: [GitLab namespaces](https://docs.gitlab.com/user/namespace/?utm_source=chatgpt.com).

## **1.5 Understanding the DevSecOps platform**

DevSecOps combines three disciplines:

- **Dev:** Software development.
- **Sec:** Software security.
- **Ops:** Software operations and deployment.

Traditional software development often treats these as separate activities.

DevSecOps integrates them into the development process.

For example, instead of waiting until a product is finished to check for security problems, GitLab can run security checks as part of the CI/CD pipeline.

[GitLab's Security Officer on an Easier Path to App Security - The New Stack](https://images.openai.com/static-rsc-4/TDyD0Sw2bDfhPpDYAtpbINDPJ6YrLZi56TFzAz4d6So24txZV8nW1ItlcrmNAD6aih7lNmdyppdlrgmQu0uCjXfa2-NDjJ3XeHBYlP2dQ_3i6RqqWna50gsDYFYU3BbeWhyEm-ugPtsJtoACk2ftOb-rLX6AMciHv7RjwYckTeA?purpose=inline)

[thenewstack.io](https://thenewstack.io/gitlabs-security-officer-on-an-easier-path-to-app-security/?utm_source=chatgpt.com)

A simplified workflow:

| Stage   | Activity                                   | GitLab feature                        |
| ------- | ------------------------------------------ | ------------------------------------- |
| Plan    | Define requirements and tasks              | Issues, Epics, Milestones             |
| Create  | Write and manage code                      | Repository, Branches                  |
| Review  | Check proposed changes                     | Merge Requests                        |
| Verify  | Build and test software                    | CI/CD Pipelines                       |
| Secure  | Scan code and dependencies                 | Security scanning                     |
| Package | Store software packages                    | Package Registry                      |
| Release | Prepare software releases                  | Releases                              |
| Deploy  | Publish software                           | GitLab Pages, deployments             |
| Monitor | Review project and operational information | Analytics and monitoring integrations |

This is the central idea behind GitLab's integrated DevSecOps platform.

Official reference: [GitLab Platform](https://about.gitlab.com/platform/?utm_source=chatgpt.com).

# **PART 2 — QUICK TOUR OF GITLAB FEATURES**

## **2.1 Understanding the GitLab dashboard**

[Inside the vision for GitLab’s new platform navigation - Self-managed - GitLab Forum](https://images.openai.com/static-rsc-4/C2QPPjYQlA7RU88zalBgv390mNq_J0Qtdu6NcVNOXzUCixgYn_y8tnLmcRplA66p83th7NTl0xmBLfCHC4-LNn-rsxXOmfUHi5L93nEq_SUL7RPkvciF95xZ5PeOT4ieWCrj9-aNSCjNfh4pzWgYKcIPbWiXKJr4yujHXxc2MD8?purpose=inline)

[forum.gitlab.com](https://forum.gitlab.com/t/inside-the-vision-for-gitlab-s-new-platform-navigation/92851?utm_source=chatgpt.com)

After logging into [GitLab.com](https://gitlab.com/?utm_source=chatgpt.com), you arrive at the GitLab interface.

The main interface contains:

| Area              | Purpose                                               |
| ----------------- | ----------------------------------------------------- |
| Top navigation    | Search, create new resources, access account settings |
| Left sidebar      | Navigate between GitLab features                      |
| Main content area | Display the selected dashboard, project, or group     |
| Search or go to   | Find projects, groups, issues, and other resources    |
| User avatar       | Profile, preferences, and account settings            |

The sidebar changes depending on whether you are viewing your personal dashboard, a group, or a project.

**Important:** GitLab occasionally changes its interface. The navigation names below refer to the current feature organisation; some items may be hidden inside expandable menus.

## **2.2 Dashboard, Groups, and Projects**

**Dashboard**

The dashboard gives you an overview of your GitLab activity and provides access to your projects and groups.

**Where to find it:** GitLab home page or dashboard.

**When to use it:** When you want to find your ESP32 projects or check recent activity.

**Groups**

Groups organise projects and users.

Where to find it:

- Top navigation: Groups.
- You can also access groups through the dashboard.

Example:

```
ESP32-Training
    ├── GPIO-Labs
    ├── WiFi-Labs
    ├── BLE-Labs
    └── OTA-Labs
```

**Projects**

Projects contain your repository and development tools.

Where to find it:

- Top navigation: Projects.
- Your personal or group dashboard.

Example: Create a project called `ESP32-Secure-OTA` to manage firmware source code, issues, and CI/CD workflows.

Official documentation: [Projects](https://docs.gitlab.com/user/project/?utm_source=chatgpt.com).

## **2.3 Issues, Issue Boards, Labels, Milestones, and Epics**

[GitLab - selfhost.cloud](https://images.openai.com/static-rsc-4/ya2WE2FtAiwf4c4wNHzp0Il1-rrBeddvkKymTmZXkF1WgvJ1aTb6u2kvWQ1noA05OwbKZsj2ZqHFIUh_SCI21SH0mqaRoTqq8DVj2whdMWXDb7KmPUs2hmANOtrd91h9KBN1eHSWiOK0HjFlbvisjk5-QA-7MNlMgGGD6jlMoyU?purpose=inline)

[GitLab vs. GitHub: Which is Better in 2026?](https://images.openai.com/static-rsc-4/kZipLqr4K7KKO3Rsln4jRKbVXvceepEwAnvKG7PtPt4Vw5riWFlv1TSQWuG7PYOublFeetz0lAFcAhP4cHTz-H38IUeT5UetSJw4uOcqhuLZGlsgD5WCwUsv4oudSCrosAtoIafh60NGosa6o5IAz4R7gS4FVsudilMbCoAU-cs?purpose=inline)

[Index · Milestones · Project · User · Help · GitLab](https://images.openai.com/static-rsc-4/BdPmmif1Y-CpMGwDMzL-tglgYHqPKyPrMh7iwosnG4SkTQjl_77wZxy_mFDVLTyu56mPY4j6KNpJOsnf5E5gQr-mfQa4lXqWwmlMRiLHmx3BXRGxsiYUwqANSdE36z0pd0wXY5WuzljV9NHMTD71HlsvlS5qidjx_liMhqkljng?purpose=inline)

8

These features help organise development work.

### **Issues**

An Issue is a record of a task, bug, improvement, or feature request.

Where to find it: Project sidebar → Plan → Work items or Issues, depending on the current interface.

Example:

```
Issue #12

Title:
Add temperature sensor support

Description:
Read temperature from the sensor every 10 seconds.

Tasks:
- Configure sensor GPIO
- Read temperature
- Print results to Serial Monitor
- Test the implementation
```

Use an Issue whenever you need to track a piece of work.

### **Issue Boards**

An Issue Board displays tasks as cards in columns, similar to a Kanban board.

Kanban is a method of organising work according to its progress.

Example:

**ESP32 development board**

To Do

Add sensor support

Doing

Implement Wi-Fi

Done

Configure GPIO

Where to find it: Plan → Issue boards.

Use it when you want to see which tasks are waiting, in progress, or completed.

GitLab Free supports Issue Boards, with one group Issue Board; some advanced board configuration is paid-tier functionality.

### **Labels**

Labels are tags attached to Issues and Merge Requests.

Examples:

- `bug`
- `enhancement`
- `documentation`
- `priority::high`
- `ESP32`
- `OTA`

Where to find it: Manage → Labels, or from an Issue.

Use labels to filter and categorise development tasks.

### **Milestones**

A Milestone groups issues and Merge Requests around a target date or development objective.

Example:

```
Milestone: ESP32 Course Release 1

Due date: 31 December 2026

Issues:
- Complete GPIO tutorial
- Complete Wi-Fi tutorial
- Complete BLE tutorial
- Complete OTA tutorial
```

Where to find it: Plan → Milestones.

### **Epics**

An Epic is a higher-level work item that groups related work across multiple projects.

Example:

```
Epic: ESP32 IoT Platform

    ├── Wi-Fi connectivity
    ├── BLE connectivity
    ├── Sensor management
    └── Secure OTA
```

Where to find it: Group-level Plan → Work items or Epics, depending on the interface.

Epics and their advanced planning capabilities have subscription-tier restrictions. Check [GitLab work item documentation](https://docs.gitlab.com/user/work_items/?utm_source=chatgpt.com) for the current availability.

## **2.4 Merge Requests, Code Review, Approvals, and Conflicts**

[DevSecOps統合プラットフォーム | GitLab](https://images.openai.com/static-rsc-4/CfKeGkCouVnY3uruczwm9iHvl72LL5ZR_XfarbX76CgtF8gb_DEBfa513YGXnVA0Zv5yol6hd9yjgb_BkKWd5vEBDkwQP-EKjzNN8vdoGWYd8DoC5ukIxA9MurOv28FEiT6VjhMsVJstqDzyxGxNel0f3r97NdzR1ydJXJ5pkGU?purpose=inline)

[dts-insight.co.jp](https://www.dts-insight.co.jp/sp/gitlab/index.html?utm_source=chatgpt.com)

A Merge Request (MR) is one of GitLab's most important collaboration features.

It allows developers to propose changes from one branch to another.

Example:

```
main
 │
 ├── feature/sensor
 │
 └── feature/wifi
```

A developer works on `feature/sensor`, pushes the changes, and opens a Merge Request to merge them into `main`.

The reviewer can:

- Examine the changed files.
- Comment on individual lines.
- Request modifications.
- Discuss implementation details.
- Approve the changes where supported.
- Merge the changes.

**Where to find it:** Project → Code → Merge requests.

### **Code review**

Code review means another developer examines your changes before they are incorporated into the main codebase.

Example:

A developer writes:

```
int temperature = readTemperature();
```

A reviewer might ask whether sensor reading failures are handled correctly.

### **Approvals**

Approvals provide an additional review mechanism.

GitLab Free allows eligible users with at least the Developer role to approve Merge Requests, but these approvals are optional and do not block merging.

Premium and Ultimate provide more flexible required approval rules.

### **Resolving conflicts**

A merge conflict occurs when two developers modify the same part of a file and Git cannot automatically combine their changes.

For example:

Developer A changes:

```
int sensorPin = 4;
```

Developer B changes:

```
int sensorPin = 5;
```

Git needs a human to decide which value is correct.

Conflicts can be resolved locally or through GitLab's Merge Request interface when supported.

Official documentation: [Merge Requests](https://docs.gitlab.com/user/project/merge_requests/?utm_source=chatgpt.com).

## **2.5 CI/CD: Pipelines, Jobs, Schedules, Variables, and Runners**

[Building Your First GitLab Pipeline: A Step-by-Step Tutorial🚀🚀🚀 - DEV Community](https://images.openai.com/static-rsc-4/0zDUgZpSjhaPuBdLIt7cRQDPX7C3ANpzl5wOBPqkNgiNgeKG86CykFCGNMJr6kHfGIyi2A1Pz_DKOH7WvuOHzEZGezyhMmVw9D279MejIyNRSr05nL-Ew3k2xJ_AImdQd_ZU2cWztMUW7CNosnocpk9A3G7_oV-SJtc80Yh9DCI?purpose=inline)

[dev.to](https://dev.to/ankitdevcode/building-your-first-gitlab-pipeline-a-step-by-step-tutorial-563i?utm_source=chatgpt.com)

CI/CD is one of GitLab's most powerful features.

**Continuous Integration (CI)** means automatically building and testing your software when code changes.

**Continuous Delivery/Deployment (CD)** means preparing or automatically deploying software after it passes the required checks.

### **Pipeline**

A pipeline is the complete automated workflow.

Example:

```
Developer pushes code
        |
        v
    Pipeline
        |
        v
   Build Stage
        |
        v
    Test Stage
        |
        v
   Deploy Stage
```

Where to find it: Build → Pipelines.

### **Jobs**

A job is one individual task in a pipeline.

Example:

```
Pipeline
    ├── lint
    ├── unit-test
    └── deploy
```

Where to find it: Build → Jobs or through the pipeline details.

### **Schedules**

Pipeline schedules allow you to execute pipelines automatically at specified times.

Example:

Run a firmware verification pipeline every night at 2:00 AM.

Where to find it: Build → Pipeline schedules.

### **Variables**

Variables store configuration values used by CI/CD jobs.

For example:

```
WIFI_SSID
API_URL
FIRMWARE_VERSION
```

Sensitive information such as access tokens should be stored as protected or masked CI/CD variables where appropriate, rather than written into source code.

Where to find it: Settings → CI/CD → Variables.

### **Runners**

A Runner executes the jobs defined in your pipeline.

Think of it as a computer that receives instructions from GitLab and runs them.

GitLab.com provides hosted runners, which are enabled by default for projects and do not require you to install a runner.

You can also configure your own self-managed runner.

Where to find it: Build → Runners.

For an ESP32 project, a pipeline could:

1. Check source-code formatting.
2. Compile the Arduino or ESP-IDF project.
3. Run software tests.
4. Generate firmware binaries.
5. Publish the firmware as a package or build artifact.

Official documentation:

- [CI/CD overview](https://docs.gitlab.com/ci/?utm_source=chatgpt.com)
- [Pipelines](https://docs.gitlab.com/ci/pipelines/?utm_source=chatgpt.com)
- [Runners](https://docs.gitlab.com/ci/runners/?utm_source=chatgpt.com)

## **2.6 Source Code: Repository, Branches, Tags, and Commits**

[softwareadvice.com.au](https://www.softwareadvice.com.au/software/28004/gitlab?utm_source=chatgpt.com)

This is where your actual source code is stored.

Where to find it: Project → Code.

### **Repository**

Displays your project files and directories.

Example:

```
ESP32-Sensor/
    ├── src/
    │   └── main.cpp
    ├── include/
    │   └── config.h
    ├── README.md
    └── platformio.ini
```

### **Branches**

Branches allow developers to work on different features without directly modifying the main branch.

Where to find it: Code → Branches.

### **Tags**

Tags identify particular commits, usually to mark releases.

Example:

```
v1.0.0
v1.1.0
v2.0.0
```

Where to find it: Code → Tags.

### **Commits**

A commit records a set of changes.

Example:

```
Commit: Add temperature sensor support

Author: Developer A
Date: 24 November 2026
Files changed: 3
```

Where to find it: Code → Commits.

## **2.7 Container Registry and Package Registry**

[How to use GitLab Container Registry - Knowledge Base of NFDI4Ing](https://images.openai.com/static-rsc-4/iLgdv3RX1CKKk-HoSiIcUAac6uiCtF_tYZPFybdNSzKTgTChHt7gN54EwRPcn__5kspUpcwXhCqYIeUUy7-36tHVj3VM0kujnvGQA_Q94-ngICt2RstDgfiZXkx8gjKjPWJu3i0oZ1p_12m5g1-WqpzPBMtCyprCuHOmkDwLu6c?purpose=inline)

[Gitlab Package Registryを利用してプライベートNPMパッケージを運用する](https://images.openai.com/static-rsc-4/aH39aHCXmshCaY2COuUu3XL-bpa3WLV5dWUX6e2CgBfTEDeq_5leqiv1_JVSw07cBuDS-0hq4bdaojlqur1UX16SuuQuNDYidiiorl6hi_Pm8mRqSA8bk803fpmMVqhu-ezDBELKe71817geFFB2rABxiXRXkqj7JXZP9sJg8rw?purpose=inline)

[reveal-md](https://images.openai.com/static-rsc-4/QYp7VFPgLdIlKHrvw4Zt9McbhWA59Hjcts2RxxWimhKOYjdeLwQ99j692N0fDFAaYbVWFR0svVUU09Z_xPULGCQzTgqTU8z8gZD43Jd16GXsMaf8tXKIFk-MhbahnAjJRdfNiXzBTAd8_qP-X-zmzSeDT6pGkSpZxdalyJcylaw?purpose=inline)

7

These registries store software outputs that can be distributed and reused.

### **Container Registry**

A container is a packaged application together with its required runtime environment.

A Container Registry stores container images, commonly Docker images.

Example:

```
registry.gitlab.com/mygroup/myproject/esp32-build:latest
```

Use it when you want to store a reusable software build environment or Docker image.

Where to find it: Deploy → Container Registry.

### **Package Registry**

A Package Registry stores software packages in supported package formats.

Examples include:

- npm packages.
- Maven packages.
- PyPI packages.
- Generic packages.

For an ESP32 project, Generic Packages are particularly useful because they can store arbitrary files.

For example:

```
firmware/
    ├── firmware.bin
    ├── version.txt
    ├── firmware.sha256
    └── firmware.sig
```

These files can be published as Generic Packages and retrieved through GitLab's Package Registry API.

Where to find it: Deploy → Package Registry.

This is useful for firmware distribution and OTA workflows.

Official documentation:

- [Container Registry](https://docs.gitlab.com/user/packages/container_registry/?utm_source=chatgpt.com)
- [Package Registry](https://docs.gitlab.com/user/packages/package_registry/?utm_source=chatgpt.com)
- [Generic Packages](https://docs.gitlab.com/user/packages/generic_packages/?utm_source=chatgpt.com)

## **2.8 Wiki, Snippets, and Web IDE**

[Blog - Using diagrams with GitLab](https://images.openai.com/static-rsc-4/T-nK1WzNU4BzDyesCG7jEThAi2KoIjHroJaBr_sCVS4GP5TIGlTwV9zVMjxzcVkff-FBySwUyMlFqZ1JeUvnwDRZMP18Qg3CpiKl0B8A8XSRr1Z83qTWU7osVrZSMX4N1Y9_P6v08OM2JinhdXksh2yoJzXaikd2V_rjYKf9y24?purpose=inline)

[Snippets · User · Help · GitLab](https://images.openai.com/static-rsc-4/HGrRYu7H23TTrqKMdlGJXmcGwAvb4q665WduoxL5B9_tUwL9bC6M2kr3QCYJ7oQrw-SCMqJva20lau_WLsSQtPFwryyzsJJLI-cVtILBxwNbkWWq1LP1h_LsS4uHoZ5JgWD0MUOCHGkzuAHSHFwWGLbhZ--tF-p2sjMRsUzDb88?purpose=inline)

[GitLab 13.0 released with Gitaly Clusters, Epic Hierarchy on Roadmaps, and Auto Deploy to ECS | GitLab](https://images.openai.com/static-rsc-4/C_0uAXOA6MDub0N2pfZ4gwf-bXR4iT0cPyDCUKYpM8fxR3rm1f0lojlRlF23m2Ls1fkTG47E7FDVbMOnBThLwpate5OU3Mut-ta5fiQY5E5EmOkLKkcFhL5EwH3owK9Oi--0F4dHY-168u4tPcIQ8xGD0_pzNIUxGSRHWG58DNk?purpose=inline)

5

### **Wiki**

A Wiki is a documentation area for your project.

Example:

```
ESP32-Sensor Wiki

    ├── Hardware Setup
    ├── GPIO Configuration
    ├── Sensor Installation
    └── Troubleshooting
```

Where to find it: Plan → Wiki.

Use it to document installation instructions or hardware connections.

### **Snippets**

Snippets allow you to share small pieces of code or text.

Example:

A small ESP32 function that calculates temperature averages.

Where to find it: Your GitLab profile or project Snippets, depending on the snippet type.

Use it to share reusable code without creating a complete project.

### **Web IDE**

The Web IDE allows you to edit repository files directly through your browser.

Where to find it: Open a project and select the Web IDE option from the repository interface.

Use it when you need to make a quick change without opening Arduino IDE or VS Code.

For serious embedded development, continue using your local development environment and Git for compiling and testing hardware-dependent code.

Official documentation:

- [Wiki](https://docs.gitlab.com/user/project/wiki/?utm_source=chatgpt.com)
- [Snippets](https://docs.gitlab.com/user/snippets/?utm_source=chatgpt.com)
- [Web IDE](https://docs.gitlab.com/user/project/web_ide/?utm_source=chatgpt.com)

## **2.9 Security and Compliance**

[Vulnerability risk prioritization made simple with GitLab](https://images.openai.com/static-rsc-4/OjHsuT8I9K1WocS389jQDCKVM9A1i7EgAI7xvKkDdohJrMUC1oDb8vFUWBcq2NIf9nh8dmx0pWnKRU6Cl4d3H6RO4bi6_M9URbpatHi23laHykpm1ZiGUIeD5giyRTWq6dQNlzSEHLdauqpnjSgVUc8d8gZNNHMFDpiGck6td9U?purpose=inline)

[about.gitlab.com](https://about.gitlab.com/blog/vulnerability-risk-prioritization-made-simple-with-gitlab/?utm_source=chatgpt.com)

GitLab integrates security checks into the development process.

Three useful features are:

### **Dependency Scanning**

A dependency is an external library or software component used by your application.

For example:

An ESP32 application might use an external JSON library.

Dependency Scanning checks supported dependencies for known security vulnerabilities.

Use it when you want to identify vulnerable third-party software.

### **Secret Detection**

Secret Detection looks for sensitive information accidentally committed to your repository.

Examples:

```
API keys
Access tokens
Passwords
Private credentials
```

This is particularly important for projects containing cloud credentials or OTA access tokens.

GitLab provides pipeline secret detection on Free, Premium, and Ultimate tiers. However, some advanced detection and reporting capabilities require Ultimate.

**Important:** Detection is not a guarantee that every secret will be found. Never commit credentials in the first place.

### **Code Quality**

Code Quality checks source code for maintainability problems and coding issues.

It can help developers identify:

- Potentially problematic coding patterns.
- Maintainability issues.
- Code-quality regressions.

Availability depends on the specific feature and subscription tier. Some advanced security and reporting functionality is restricted to paid tiers.

### **Where to find security features**

Project sidebar → Secure, or the relevant Security and Compliance sections.

The exact menu and available reports depend on your GitLab version, subscription, and enabled scanners.

Official documentation:

- [Application Security](https://docs.gitlab.com/user/application_security/?utm_source=chatgpt.com)
- [Dependency Scanning](https://docs.gitlab.com/user/application_security/dependency_scanning/?utm_source=chatgpt.com)
- [Secret Detection](https://docs.gitlab.com/user/application_security/secret_detection/?utm_source=chatgpt.com)
- [Code Quality](https://docs.gitlab.com/ci/testing/code_quality/?utm_source=chatgpt.com)

## **2.10 Settings: Members, Access Levels, Protected Branches, and Integrations**

[Gitlab - Parcours DevOPS](https://images.openai.com/static-rsc-4/_9fRYQ0Xe8AqsHma3zBcfmk097-j7ZAIJufGFmuRbnXGrl66IzrdHt7h7LksTm4I_lTXswJvz8PLGJdUZeBoKSXSN-Wu515mHjomXxFqeWFSy9HqaGdphCX2vyXMs-v1CvUwsO7S4qNwT0EwYjZU-SErOFNtYbEqVdolWcFxoWk?purpose=inline)

[parcours-devops.gitlab-pages.imt-atlantique.fr](https://parcours-devops.gitlab-pages.imt-atlantique.fr/gitlab/gitlab/?utm_source=chatgpt.com)

### **Members and access levels**

You can invite other developers to your project or group.

GitLab has several roles:

| Role       | Main permissions                                               |
| ---------- | -------------------------------------------------------------- |
| Guest      | Limited access to project information and interactions         |
| Planner    | Manage planning work items, with restricted code permissions   |
| Reporter   | Read repository and contribute to permitted project activities |
| Developer  | Push code to non-protected branches and create Merge Requests  |
| Maintainer | Manage most project settings, branches, and project resources  |
| Owner      | Full group or namespace administration                         |

Some roles and capabilities vary by project type and subscription.

Where to find it: Manage → Members or Project information → Members, depending on the interface.

### **Protected branches**

A protected branch restricts who can push directly to or merge into an important branch.

Example:

```
main
    Protected

feature/sensor
    Normal branch

feature/wifi
    Normal branch
```

Developers work on feature branches and submit Merge Requests to `main`.

Where to find it: Settings → Repository → Branch rules or Protected branches.

### **Integrations**

Integrations connect GitLab to external services.

Examples include:

- External notification services.
- Issue-tracking integrations.
- Monitoring and development tools.
- Webhooks.

A webhook is an automated notification sent from GitLab to another application when an event occurs.

Where to find it: Settings → Integrations.

Official documentation: [GitLab project settings](https://docs.gitlab.com/user/project/settings/?utm_source=chatgpt.com).

## **2.11 GitLab Free vs Paid: What should beginners know?**

GitLab has Free, Premium, and Ultimate subscription tiers.

The Free tier is sufficient for learning Git, managing personal repositories, creating Issues and Merge Requests, and experimenting with CI/CD.

However, Free has limits.

**GitLab.com Free — important limits**

**Repository and LFS storage**

# **10 GiB**

Per project in a Free namespace.

**Private top-level namespace members**

# **5**

For newly created private top-level namespaces; membership is counted across the namespace.

**Top-level groups**

# **3**

For Free accounts created after 27 January 2026. Personal namespaces are excluded.

Other important considerations:

| Feature                          | Free tier considerations             |
| -------------------------------- | ------------------------------------ |
| Git repositories                 | Available                            |
| Private and public projects      | Available                            |
| Issues                           | Available                            |
| Issue Boards                     | Available, with limitations          |
| Merge Requests                   | Available                            |
| Merge Request approvals          | Optional approvals, not enforced     |
| CI/CD                            | Available, subject to compute limits |
| GitLab-hosted runners            | Available, subject to compute quota  |
| GitLab Pages                     | Available                            |
| Package Registry                 | Available                            |
| Container Registry               | Available                            |
| Advanced approval rules          | Paid tiers                           |
| Advanced security and compliance | Some capabilities require Ultimate   |

### **CI/CD compute minutes**

GitLab-hosted runners consume compute minutes allocated to your namespace.

The available allowance depends on the current subscription and applicable GitLab.com quota.

Self-managed runners have different resource and billing considerations.

Check your actual allowance under:

**Group or personal namespace → Settings → Usage quotas.**

Do not assume that all CI/CD execution is unlimited just because pipelines are available on Free.

### **Storage**

GitLab Free provides 10 GiB per project for repository and Git LFS storage.

When a project exceeds its applicable storage limit, it can become read-only.

Container Registry, Package Registry, and build artifacts are not included in that particular repository/LFS storage calculation.

Official references:

- [GitLab Free user and group limits](https://docs.gitlab.com/user/free_user_limit/?utm_source=chatgpt.com)
- [GitLab storage limits](https://docs.gitlab.com/user/storage_usage_quotas/?utm_source=chatgpt.com)
- [GitLab compute minutes](https://docs.gitlab.com/ci/pipelines/compute_minutes/?utm_source=chatgpt.com)
- [GitLab pricing](https://about.gitlab.com/pricing/?utm_source=chatgpt.com)

# **PART 3 — STEP-BY-STEP SETUP INSTRUCTIONS**

We will create a GitLab project called `ESP32-Learning-Lab`.

The example uses:

- GitLab.com.
- Windows PowerShell.
- Git installed on your computer.
- HTTPS or SSH authentication.
- A private project initially.

You can substitute your own project name.

## **3.1 Step 1 — Create a GitLab account**

[Presentándose y Entendiendo GGG (GuayaHack, Git, GitLab) — HackerSpace y Grupo de estudio](https://images.openai.com/static-rsc-4/buPBk8WWqeovuf87V9iBwQj80Ddw7heBzwrrR-VkmXdAJ2pq29O7tYGd-BC0GiUzw2XbREWKpK2b2jM4drcyieLolzEdUkNsqxIxJuXvoHxd_IHMb_0hWLdQQOuj9AIRg7L3p18ueoTj0LUKpvQXmsHLQEe7uBALWlyCLUGQRRk?purpose=inline)

[guayahack.co](https://guayahack.co/reto/on-boarding-git-gitlab/?utm_source=chatgpt.com)

1. Open [https://gitlab.com](https://gitlab.com?utm_source=chatgpt.com).
2. Select **Register**.
3. Enter your email address.
4. Create a username.
5. Set a strong password.
6. Complete any verification requirements.
7. Sign in.

**What you should see:**

Your GitLab dashboard, with access to projects, groups, and other GitLab features.

Your username will form part of your personal GitLab URL.

For example:

```
https://gitlab.com/your-username
```

Keep your username simple because it will appear in repository URLs.

## **3.2 Step 2 — Set up your profile and authentication**

### **A. Configure your profile**

1. Select your profile avatar.
2. Open **Edit profile** or Preferences.
3. Enter your name.
4. Add an optional profile description.
5. Set your preferred language and other preferences.
6. Save your changes.

### **B. Choose HTTPS or SSH authentication**

GitLab supports two common ways of authenticating Git operations.

| Method | Description                                            | Recommendation                     |
| ------ | ------------------------------------------------------ | ---------------------------------- |
| HTTPS  | Connect using an HTTPS URL and a Personal Access Token | Easier for beginners               |
| SSH    | Connect using a cryptographic SSH key pair             | Convenient for regular development |

A Personal Access Token (PAT) is a secret credential that allows applications such as Git to authenticate with GitLab.

**Option 1: HTTPS authentication**

1. Open your GitLab avatar.
2. Select **Edit profile**.
3. Open **Access → Personal access tokens**.
4. Create a token.
5. Give it a descriptive name:

```
Windows-Git-Development
```

6. Set an expiry date.
7. For ordinary Git repository operations, select the `write_repository` scope. The `read_repository` scope is sufficient for read-only operations.
8. Create the token.
9. Copy and securely store it immediately.

Important security advice

GitLab displays the token when it is created. Do not commit it into your repository or share it with other people.

When Git asks for credentials:

- Username: Your GitLab username.
- Password: Your Personal Access Token.

Use Windows Credential Manager or Git Credential Manager to securely retain credentials.

**Option 2: SSH authentication**

First, check whether you already have an SSH key.

Open Windows PowerShell:

```
Get-ChildItem ~/.ssh
```

If you do not have an existing suitable key, generate one:

```
ssh-keygen -t ed25519 -C "your-email@example.com"
```

Press Enter to accept the default key location.

You can also set a passphrase for additional protection.

Start the SSH agent:

```
Get-Service ssh-agent | Set-Service -StartupType Automatic
Start-Service ssh-agent
```

Add your private key:

```
ssh-add $env:USERPROFILE\.ssh\id_ed25519
```

Display your public key:

```
Get-Content $env:USERPROFILE\.ssh\id_ed25519.pub
```

Copy the complete output.

Now in GitLab:

1. Select your avatar.
2. Open **Edit profile**.
3. Select **Access → SSH keys**.
4. Paste your public key.
5. Add a descriptive title.
6. Save the key.

Test your connection:

```
ssh -T git@gitlab.com
```

You should receive a welcome message identifying your GitLab username.

**Never upload or share&#x20;**`id_ed25519`**&#x20;(the private key). Only upload&#x20;**`id_ed25519.pub`**&#x20;(the public key).**

Official instructions:

- [GitLab SSH keys](https://docs.gitlab.com/user/ssh/?utm_source=chatgpt.com)
- [Personal Access Tokens](https://docs.gitlab.com/user/profile/personal_access_tokens/?utm_source=chatgpt.com)

## **3.3 Step 3 — Create your first Group and Project**

### **A. Create a Group**

A Group organises multiple related projects and their members.

1. From the top navigation, select **Groups**.
2. Select **New group** or **Create group**.
3. Select **Create group**.
4. Enter the group name:

```
ESP32-Learning
```

5\. Set the group URL:

```
esp32-learning
```

6. Choose the visibility level.
7. Select **Create group**.

[Creating projects on GitLab](https://images.openai.com/static-rsc-4/WwnxzkMRBG1-P3m2CwZUsHX1Hz-HeUz2nLC1jeObW4NKX4fM5GbUjjxJmULVlD3flAnt5AD2ejm5uhie0Dx1tjj8sJkdmJu3A3P8EcdQyGZtXnn0oys-g-i-mW2sg6e40xpEZnKVISySsE0eIC7DQb0aUyvwg8MrSBj7gm_SJ70?purpose=inline)

[tutorial.gitlabpages.inria.fr](https://tutorial.gitlabpages.inria.fr/git/02-creating-projects/?utm_source=chatgpt.com)

You should now see your group overview page.

### **B. Create a Project**

1. From the group page, select **New project**.
2. Select **Create blank project**.
3. Enter:

```
Project name: ESP32-Learning-Lab
Project slug: esp32-learning-lab
```

4. Select your group as the project namespace.
5. Choose the visibility.
6. Optionally select **Initialize repository with a README**.
7. Select **Create project**.

[Gitlab - Parcours DevOPS](https://images.openai.com/static-rsc-4/J6ap_k0LzRk3Q04KooJTjuhIc9Xe1Is9J7POxSNKz1k36vo92fVmAaq-81cejAPNj1QVqH5qr2jQFakeg_t3OcBr-AePySBC79dshgafZSZy8055w51zXx8VjuPbDMePx17NhusbBPw8Lq-xYLXzTjUUqm2RfiDnNOGTeXy99wo?purpose=inline)

[parcours-devops.gitlab-pages.imt-atlantique.fr](https://parcours-devops.gitlab-pages.imt-atlantique.fr/gitlab/gitlab/?utm_source=chatgpt.com)

You should see the project overview.

### **Understanding Group, Project, and Namespace**

| Item      | Purpose                                          | Example              |
| --------- | ------------------------------------------------ | -------------------- |
| Namespace | Identifies the account or group owning a project | `esp32-learning`     |
| Group     | Organises projects and users                     | `ESP32-Learning`     |
| Project   | Manages one software development workspace       | `ESP32-Learning-Lab` |

The resulting project URL should look like:

```
https://gitlab.com/esp32-learning/esp32-learning-lab
```

A group can also contain subgroups, allowing you to organise projects into multiple levels.

For example:

```
ESP32-Learning
    |
    ├── Beginner-Course
    |      ├── GPIO-Labs
    |      └── WiFi-Labs
    |
    └── Advanced-Course
           ├── FreeRTOS
           └── Secure-OTA
```

## **3.4 Step 4 — Choose project visibility**

GitLab provides three main visibility levels.

| Visibility | Who can access it?                         | Suitable use                       |
| ---------- | ------------------------------------------ | ---------------------------------- |
| Private    | Only authorised members                    | Personal code and company projects |
| Internal   | Authenticated users of the GitLab instance | Sharing within an organisation     |
| Public     | Anyone, including unauthenticated visitors | Open-source projects               |

[Make prive repo public - How to Use GitLab - GitLab Forum](https://images.openai.com/static-rsc-4/p4G5vRq_TCBhZgbPmoXcVgDM619nhxlGZnsVD-iuZGsPGFINY89eS6jPPVpjUujtyDwOpnQBk4VCZyRcRew8R0pI_9MQCXhiWy0MQOFsbF21jAJUqyNy6bIrJ6CoFI9m3mPaF0oXFifTsP9jydq3ULlWvwR1nPSjZqLLCEzXNco?purpose=inline)

[forum.gitlab.com](https://forum.gitlab.com/t/make-prive-repo-public/17965?utm_source=chatgpt.com)

### **Private**

Choose Private if:

- Your firmware contains confidential information.
- You are developing proprietary software.
- You only want invited members to access the project.

### **Internal**

Internal visibility is mainly relevant to GitLab Self-Managed environments where authenticated users can access internal projects.

On GitLab.com, Internal visibility is not available in the same way as on Self-Managed installations.

### **Public**

Choose Public if you want anyone to view and clone your repository.

**My recommendation:** Start with Private while learning. You can change the visibility later.

Important: Making a project private does not automatically protect credentials that have already been committed into its Git history.

## **3.5 Step 5 — Clone the project to your computer**

Cloning means downloading a Git repository and its history to your local computer.

Open your project in GitLab.

1. Select the **Code** dropdown.
2. Copy either the HTTPS or SSH repository URL.

[Erste Schritte mit GitLab: So importierst du deine Projekte in GitLab](https://images.openai.com/static-rsc-4/_2p4ISshKAW7aFRMU7kHV2ese0NB4rYuiCgzU_Q0t2M33JcVg_DKcdbyHBtSng8KpMlHDJAczr5TT9Y4YarNp1gfYgisejCDD4ynZIV1ZT5jatW_wEw8x6Wn2-Xwu-VRR58u6Rl3mjAUNWcmRvFnha8voQ_PrMppZPEuhcPaHS8?purpose=inline)

[about.gitlab.com](https://about.gitlab.com/de-de/blog/getting-started-with-gitlab-how-to-import-your-projects-to-gitlab/?utm_source=chatgpt.com)

Example HTTPS URL:

```
https://gitlab.com/esp32-learning/esp32-learning-lab.git
```

Example SSH URL:

```
git@gitlab.com:esp32-learning/esp32-learning-lab.git
```

Open PowerShell and navigate to your working directory:

```
cd C:\Users\YourUser\Documents
```

Clone using HTTPS:

```
git clone https://gitlab.com/esp32-learning/esp32-learning-lab.git
```

Or clone using SSH:

```
git clone git@gitlab.com:esp32-learning/esp32-learning-lab.git
```

Enter the directory:

```
cd esp32-learning-lab
```

Check the repository:

```
git status
```

Expected output:

```
On branch main
Your branch is up to date with 'origin/main'.

nothing to commit, working tree clean
```

The exact output depends on whether you initialized the project with a README.

## **3.6 Step 6 — Add files, commit, and push your first change**

Create a README file if you have not already created one.

A README is the main project introduction. GitLab displays it on your project overview page.

In PowerShell:

```
@"
# ESP32 Learning Lab

This project contains ESP32 learning materials.

## Topics
- GPIO
- Wi-Fi
- Bluetooth
- FreeRTOS
- IoT
- Secure OTA
"@ | Set-Content README.md
```

Check the files:

```
Get-ChildItem
```

Check Git:

```
git status
```

Add the README:

```
git add README.md
```

Commit:

```
git commit -m "Add project README"
```

Push to GitLab:

```
git push -u origin main
```

Expected output will indicate that your commit was uploaded and the remote branch was updated.

### **If the push fails**

| Error                         | Possible reason                    | Solution                          |
| ----------------------------- | ---------------------------------- | --------------------------------- |
| Authentication failed         | Invalid HTTPS credentials          | Use a valid PAT                   |
| Permission denied (publickey) | SSH key not configured             | Check SSH key and agent           |
| Rejected non-fast-forward     | Remote has commits missing locally | Pull and integrate remote changes |
| Project not found             | Incorrect URL or no access         | Verify project URL and membership |

If you created a README through GitLab and already have local files, you may need to pull the initial remote commit before pushing.

For example:

```
git pull --rebase origin main
git push -u origin main
```

If Git reports conflicts, resolve them before continuing.

## **3.7 Step 7 — Check your project in GitLab**

Return to your project:

[https://gitlab.com/esp32-learning/esp32-learning-lab](https://gitlab.com/esp32-learning/esp32-learning-lab?utm_source=chatgpt.com)

You should now see:

[Introduction to GitLab: Adding Files to a Project Repository](https://images.openai.com/static-rsc-4/0LYh_yCAXGsuV8PtDzSjrMe_OecPMWHzlQbbd3ljnYiWL7T3R7yrBkS18WUJty0qT1E1HPB6bhrTM_lV_7nbVhLZJyUItFT20J4OwV0Rg4O89GhrMhaSWLGoDn9--wagRm25o1xAVh4kg8MYI-wXykO6v_AyJrFLGMhPZt_bPFc?purpose=inline)

[carpentries-incubator.github.io](https://carpentries-incubator.github.io/gitlab-novice/03-fill-repository.html?utm_source=chatgpt.com)

1. Your project name.
2. Your README content.
3. Your repository files.
4. The latest commit message.
5. The default branch, usually `main`.

To verify your changes:

1. Select **Code → Repository**.
2. Check that `README.md` exists.
3. Open the file.
4. Check that its contents match your local file.
5. Select **Code → Commits**.
6. Confirm that your commit appears in the history.

Congratulations! Your first local Git repository is now connected to GitLab.

## **3.8 Step 8 — Create your first branch and Merge Request**

Now we will practise the standard development workflow.

The objective is to add a new file without directly modifying `main`.

### **A. Create a branch**

In PowerShell:

```
git switch -c feature/add-project-info
```

Expected output:

```
Switched to a new branch 'feature/add-project-info'
```

### **B. Create a new file**

```
@"
# Project Information

This project demonstrates how to use GitLab
to manage ESP32 software development.
"@ | Set-Content PROJECT_INFO.md
```

### **C. Commit the change**

```
git add PROJECT_INFO.md

git commit -m "Add project information"
```

### **D. Push the branch**

```
git push -u origin feature/add-project-info
```

Your branch is now available in GitLab.

### **E. Create a Merge Request**

[Reviewing and combine versions (merge/pull request, merging) — TeachBooks Manual](https://images.openai.com/static-rsc-4/J8HYcFfS300KIKqlL7XOqTeRSWBxqiic5Z2orpin0HNgBmEXYVji39b6BA9BNORqGHL7wgJ5DfXME6nYbQmJi1fG6wx6TA9yEdYNsSU8KC6p59DeZ-ERu2pz86z7T0fkTr5gUKUzS1WgPnOcyS_NjfjjjnP6zkIOFQKA64HWgd4?purpose=inline)

[teachbooks.io](https://teachbooks.io/manual/workflows/merge_request.html?utm_source=chatgpt.com)

1. Open your GitLab project.
2. Select **Code → Merge requests**.
3. Select **New merge request**.
4. Choose the source branch:

```
feature/add-project-info
```

5\. Choose the target branch:

```
main
```

6. Select **Compare branches and continue**.
7. Enter a title:

```
Add project information
```

8. Enter a description.
9. Select **Create merge request**.

### **F. Review and merge**

GitLab displays the proposed changes.

1. Select the Changes tab.
2. Check the new file.
3. Review the diff (the comparison showing what changed).
4. If everything is correct, select Merge when available.

If you are the only developer, you can review your own work for this beginner exercise.

For a team, another developer should review the changes.

### **G. Confirm the result**

After merging:

```
git switch main
git pull origin main
```

You should now see:

```
Get-ChildItem
```

The file `PROJECT_INFO.md` should exist on your main branch.

## **3.9 Setup checklist**

Use this checklist to confirm that your GitLab environment is ready.

**GitLab setup progress**

# **0%**

Create a GitLab account

Set up your username and profile

Configure HTTPS PAT or SSH authentication

Create a GitLab Group

Create a GitLab Project

Choose the correct project visibility

Clone the project to your computer

Create README.md

Commit and push your first change

Create a feature branch

Open your first Merge Request

Review and merge the changes

Reset checklist

# **PART 4 — HANDS-ON TUTORIALS: BUILD FOUR GITLAB PROJECTS**

This section contains four practical exercises.

The tutorials build on each other:

1. **Tutorial A — Create a project**

   Create a repository and upload existing local code.
2. **Tutorial B — Collaborate using GitLab**

   Create Issues, branches, and Merge Requests.
3. **Tutorial C — Automate testing**

   Build your first CI/CD pipeline.
4. **Tutorial D — Deploy a website**

   Publish a website using GitLab Pages.

## **Tutorial A — Create a project from scratch and push existing code**

**Objective:** Create a new GitLab project, upload existing local source code, and configure a README.

### **Step A1 — Create a project**

1. Sign in to GitLab.
2. Select **New project**.
3. Select **Create blank project**.
4. Enter:

```
Project name: GitLab-Tutorial-A
Visibility: Private
Initialize repository with README: No
```

5\. Select **Create project**.

We leave the README option unchecked because we will create it locally.

### **Step A2 — Create your local project**

Open Windows PowerShell:

```
cd C:\Users\YourUser\Documents

mkdir GitLab-Tutorial-A

cd GitLab-Tutorial-A
```

Create the following directory structure:

```
GitLab-Tutorial-A/
│
├── README.md
├── main.py
└── config/
    └── settings.txt
```

Create `main.py`:

```
@'
def main():
    print("Hello from GitLab!")

if __name__ == "__main__":
    main()
'@ | Set-Content main.py
```

Create the configuration directory:

```
New-Item -ItemType Directory -Path config
```

Create `settings.txt`:

```
@'
Application=GitLab-Tutorial-A
Version=1.0.0
Environment=Development
'@ | Set-Content config/settings.txt
```

Create `README.md`:

```
@'
# GitLab Tutorial A

My first GitLab project.

## Description

This project demonstrates Git repository management.

## Files

- main.py
- config/settings.txt

## Version

1.0.0
'@ | Set-Content README.md
```

### **Step A3 — Test the Python program**

Run:

```
python main.py
```

Expected output:

```
Hello from GitLab!
```

If `python` is not recognised, install Python from [python.org](https://www.python.org/downloads/?utm_source=chatgpt.com) and ensure Python is added to PATH.

### **Step A4 — Initialise Git**

```
git init
```

Set your branch name:

```
git branch -M main
```

Add all files:

```
git add .
```

Create your first commit:

```
git commit -m "Initial project creation"
```

### **Step A5 — Connect to GitLab**

Copy the HTTPS URL from your GitLab project.

Example:

```
https://gitlab.com/your-username/gitlab-tutorial-a.git
```

Connect the remote repository:

```
git remote add origin https://gitlab.com/your-username/gitlab-tutorial-a.git
```

Push:

```
git push -u origin main
```

If you use SSH, replace the HTTPS URL with the SSH URL.

### **Step A6 — Verify in GitLab**

Open your project.

Select **Code → Repository**.

You should see:

[Formation Git/GitLab – Session 2 : GitLab et dépôt distant](https://images.openai.com/static-rsc-4/ocicIN1eES5yFLti9Aa3e27Z4JdoRLHJzw07QuhuhBPBW5jWP1rCOcHeHXNrSo-3o1lrCCglm5pNGm6UVhhtp0kFT8YAUUk4PiZ5beIKg5IzLitwSYm5OEsZE8hmYDG8uoLiB9b8rgNja0aeABDxhBdDYNoYJY2C-8-IDy7ZqMY?purpose=inline)

[formation-git-gitlab-nab.pages-forge.inrae.fr](https://formation-git-gitlab-nab.pages-forge.inrae.fr/support-de-formation-git-gitlab/archive/02_gitlab_depot_distant/?utm_source=chatgpt.com)

```
GitLab-Tutorial-A/
│
├── README.md
├── main.py
└── config/
    └── settings.txt
```

Select README.md.

GitLab should display its formatted content on the project overview page.

Tutorial A complete

You have created a GitLab project and uploaded existing local code.

## **Tutorial B — Work like a team using Issues, Branches, and Merge Requests**

**Objective:** Simulate a team development workflow.

We will create an Issue requesting a new function, develop the feature on a branch, and merge the changes into `main`.

### **Step B1 — Create an Issue**

Open your `GitLab-Tutorial-A` project.

1. Select **Plan → Work items**.
2. Select **New item**.
3. Choose Issue as the item type if prompted.
4. Enter:

```
Title:
Add a version information function

Description:
Create a function that displays the application version.

Acceptance criteria:
- Display application name.
- Display application version.
- Display the development environment.
```

5\. Select **Create issue**.

You should see a new Issue, for example:

```
Issue #1

Add a version information function

Status: Open
```

The number may differ.

### **Step B2 — Create a branch from the Issue**

This is a useful GitLab feature because it connects the development branch directly to the Issue.

1. Open your new Issue.
2. Find the **Create merge request** dropdown beneath the description.
3. Select **Create branch**.
4. Choose `main` as the source branch.
5. Use the suggested branch name or enter:

```
1-add-version-information
```

6\. Select **Create branch**.

The exact branch name may be generated using your Issue number and title.

GitLab links the branch to the Issue.

Official instructions: [Create a branch from an Issue](https://docs.gitlab.com/user/project/repository/branches/?utm_source=chatgpt.com).

### **Step B3 — Switch to the new branch locally**

In PowerShell:

```
git fetch origin
```

Switch to the branch:

```
git switch 1-add-version-information
```

If Git reports that the branch is not available locally, use:

```
git switch --track origin/1-add-version-information
```

Check your current branch:

```
git branch
```

Expected output:

```
* 1-add-version-information
  main
```

### **Step B4 — Modify the Python program**

Open `main.py` and replace its content with:

```
APP_NAME = "GitLab-Tutorial-A"APP_VERSION = "1.1.0"ENVIRONMENT = "Development"def show_version():    print(f"Application: {APP_NAME}")    print(f"Version: {APP_VERSION}")    print(f"Environment: {ENVIRONMENT}")def main():    print("Hello from GitLab!")    show_version()if __name__ == "__main__":    main()
```

Run the program:

```
python main.py
```

Expected output:

```
Hello from GitLab!
Application: GitLab-Tutorial-A
Version: 1.1.0
Environment: Development
```

### **Step B5 — Commit and push**

```
git add main.py

git commit -m "Add version information function"

git push origin 1-add-version-information
```

The changes are now stored in your GitLab feature branch.

### **Step B6 — Open a Merge Request**

1. Open the project.
2. Select **Code → Merge requests**.
3. Select **New merge request**.
4. Choose:

```
Source branch:
1-add-version-information

Target branch:
main
```

5. Select **Compare branches and continue**.
6. Enter:

```
Title:
Add application version information

Description:
Implements the version information function
for the application.

Closes #1
```

7\. Select **Create merge request**.

The `Closes #1` keyword instructs GitLab to close the associated Issue when the Merge Request is merged into the default branch, provided automatic issue closing is enabled and the project configuration supports it.

### **Step B7 — Review the changes**

[Speed up code reviews: Let AI handle the feedback implementation](https://images.openai.com/static-rsc-4/ZtlDGAjYhoNEZ2OZw-fXtJ5PVMBFJXS6jy3tQQQF5uXxlprHaqs1rSrZMt03qUDh9jWNHrq-N4uAEy7Z7SQxhRzbxR9YbGAWiJ2U45PkU1sf0mwi5rjDPSdmfggpaw2CNW9loSB1Joh-d6dPouOhIeIHhewekJb-EPYGq902vWs?purpose=inline)

[about.gitlab.com](https://about.gitlab.com/blog/speed-up-code-reviews-let-ai-handle-the-feedback-implementation/?utm_source=chatgpt.com)

Select the **Changes** tab.

You should see:

- The original code.
- The new code.
- Added and removed lines.
- The commit information.

Try adding a comment to a changed line.

For a real team:

1. Assign a reviewer.
2. Ask the reviewer to examine the changes.
3. Address any comments.
4. Obtain the required approvals, if configured.

For this Free-tier exercise, approvals are optional and do not enforce a merge restriction.

### **Step B8 — Merge**

Select **Merge** when the Merge Request is ready.

After successful merging, the feature is incorporated into `main`.

### **Step B9 — Update your local main branch**

In PowerShell:

```
git switch main

git pull origin main
```

Run the program:

```
python main.py
```

You should see the new version information.

Tutorial B complete

You have experienced the standard GitLab development workflow:

```
Issue
  |
  v
Feature Branch
  |
  v
Code Changes
  |
  v
Commit and Push
  |
  v
Merge Request
  |
  v
Code Review
  |
  v
Merge into main
  |
  v
Issue Closed
```

## **Tutorial C — Build your first CI/CD pipeline**

Important concept

**Objective:** Automatically check your source code and run tests whenever you push changes to GitLab.

We will use a simple JavaScript project running on Node.js.

You do not need to install Node.js on your computer to run the GitLab pipeline. GitLab's hosted runner will use a Node.js container image.

### **Step C1 — Understand the CI/CD workflow**

[CI/CD pipelines | GitLab Docs](https://images.openai.com/static-rsc-4/kxyNiG8QIOyE0B6Z8WizHPewG6P7Vla2LVoddAv8WytiLZTEKXNK3cEw8tq_qjovgfEkv2GDCZnR4OoOVJgRk2b8y0wW_pwpCpLYBwyd4FmsOr4r3PmC1JYnw0v7mQ0Zt741MSjnqB_3hZrkf6rse4oL8F0MBAUbhgNNYT3xcK8?purpose=inline)

[docs.gitlab.com](https://docs.gitlab.com/ci/pipelines/?utm_source=chatgpt.com)

Our pipeline will have two stages:

1. Lint: Check JavaScript syntax.
2. Test: Run a simple software test.

A **stage** is a group of related jobs.

Jobs within the same stage can run in parallel. The next stage starts after the preceding stage succeeds.

Our example:

```
Developer pushes code
        |
        v
    Pipeline
        |
        v
   LINT STAGE
        |
        v
   lint-job
        |
        v
   TEST STAGE
        |
        v
   test-job
        |
        v
   Pipeline Passed
```

### **Step C2 — Create a new GitLab project**

Create a blank project:

```
Project name: GitLab-Tutorial-CI
Visibility: Private
Initialize with README: No
```

Clone it:

```
cd C:\Users\YourUser\Documents

git clone https://gitlab.com/your-username/gitlab-tutorial-ci.git

cd gitlab-tutorial-ci
```

### **Step C3 — Create the project structure**

Our folder structure:

```
gitlab-tutorial-ci/
│
├── .gitlab-ci.yml
├── package.json
│
├── src/
│   └── calculator.js
│
└── test/
    └── calculator.test.js
```

Create the directories:

```
New-Item -ItemType Directory -Path src

New-Item -ItemType Directory -Path test
```

### **Step C4 — Create the JavaScript application**

Create `src/calculator.js`:

```
function add(a, b) {
    return a + b;
}

function subtract(a, b) {
    return a - b;
}

module.exports = {
    add,
    subtract
};
```

### **Step C5 — Create the test program**

Create `test/calculator.test.js`:

```
const assert = require("node:assert/strict");

const calculator = require("../src/calculator");

assert.equal(calculator.add(2, 3), 5);

assert.equal(calculator.subtract(5, 3), 2);

console.log("All tests passed!");
```

This test checks whether the calculator functions return the expected results.

### **Step C6 — Create package.json**

Create `package.json`:

```
{
  "name": "gitlab-tutorial-ci",
  "version": "1.0.0",
  "description": "GitLab CI/CD beginner tutorial",
  "scripts": {
    "lint": "node --check src/calculator.js",
    "test": "node test/calculator.test.js"
  }
}
```

The `lint` command checks JavaScript syntax.

The `test` command runs the test program.

We are using Node.js built-in functionality, so no additional npm packages are required.

### **Step C7 — Test locally (optional)**

If Node.js is installed locally, check its version:

```
node --version
```

Run the lint command:

```
npm run lint
```

Run the test command:

```
npm test
```

Expected test output:

```
All tests passed!
```

### **Step C8 — Create .gitlab-ci.yml**

This is the most important file in this tutorial.

The `.gitlab-ci.yml` file defines the automated pipeline.

Create it in the root directory of the project.

```

image: node:22

stages:
  - lint
  - test

lint-job:
  stage: lint
  script:
    - npm run lint

test-job:
  stage: test
  script:
    - npm test

```

Let's understand each part.

**1. image**

```
image: node:22
```

Specifies the container image used by the runner.

The image provides the Node.js environment needed by our jobs.

**2. stages**

```
stages:
  - lint
  - test
```

Defines the execution order.

Lint runs before Test.

**3. lint-job**

```
lint-job:
  stage: lint
  script:
    - npm run lint
```

Defines a job named `lint-job`.

GitLab executes `npm run lint`.

**4. test-job**

```
test-job:
  stage: test
  script:
    - npm test
```

Defines the second job.

It runs only after the lint stage succeeds.

### **Step C9 — Commit and push**

In PowerShell:

```
git add .

git commit -m "Add first GitLab CI pipeline"

git push -u origin main
```

The push automatically triggers a pipeline.

### **Step C10 — View the pipeline in GitLab**

[GitLab CI/CD Tutorial: From Basic To Advanced  | Octopus Deploy](https://images.openai.com/static-rsc-4/LQxHSSzi_R99vyu3Pttyg9QuB03i55urJkxYc-sJX0q2cHM2KRqKuieTev-tE6fRYlniowS5W3y_YtVa8FSBNs4FBDlWWRClOx4RG6b6c_t3OWq1ZN4lY6t_1NDOmBQhY66pBLKIjkd66FtWf_SHdGTPSLDhYo0qBtkaE6kyips?purpose=inline)

[octopus.com](https://octopus.com/devops/gitlab/gitlab-cicd-tutorial/?utm_source=chatgpt.com)

1. Open your project.
2. Select **Build → Pipelines**.
3. Find the latest pipeline.
4. Open the pipeline.
5. Select `lint-job`.
6. Review its execution log.
7. Select `test-job`.
8. Check the output.

Expected output:

```
$ npm run lint

> gitlab-tutorial-ci@1.0.0 lint
> node --check src/calculator.js

Job succeeded.
```

And:

```
$ npm test

> gitlab-tutorial-ci@1.0.0 test
> node test/calculator.test.js

All tests passed!

Job succeeded.
```

Pipeline passed

### **Step C11 — Experiment: Make the pipeline fail**

A good way to understand CI/CD is to deliberately introduce an error.

Modify `test/calculator.test.js`:

```
assert.equal(calculator.add(2, 3), 100);
```

Commit and push:

```
git add .

git commit -m "Test pipeline failure"

git push
```

GitLab runs the pipeline again.

This time, the test job should fail because the actual result is 5, not 100.

Open:

**Build → Pipelines → Failed pipeline → test-job**

Read the error log.

Correct the test:

```
assert.equal(calculator.add(2, 3), 5);
```

Commit and push again:

```
git add .

git commit -m "Fix calculator test"

git push
```

The pipeline should pass.

### **Step C12 — Understand additional CI/CD features**

Once the basic pipeline works, explore these capabilities:

| Feature            | What it does                             | Where to find it                         |
| ------------------ | ---------------------------------------- | ---------------------------------------- |
| Pipeline schedules | Run a pipeline at a scheduled time       | Build → Pipeline schedules               |
| CI/CD Variables    | Store configuration and secrets          | Settings → CI/CD → Variables             |
| Runners            | Manage execution environments            | Build → Runners                          |
| Job artifacts      | Save output files from jobs              | Job details → Artifacts                  |
| Pipeline editor    | Edit and validate pipeline configuration | Build → Pipeline editor, where available |

For example, an ESP32 pipeline could eventually look like:

```
Source Code
    |
    v
Code Formatting
    |
    v
Compile ESP32 Firmware
    |
    v
Unit Tests
    |
    v
Generate Firmware Binary
    |
    v
Calculate SHA-256
    |
    v
Publish Firmware Package
```

This is a practical starting point for automating embedded software development.

Official reference: [GitLab CI/CD YAML syntax](https://docs.gitlab.com/ci/yaml/?utm_source=chatgpt.com).

## **Tutorial D — Deploy a static website using GitLab Pages**

**Objective:** Publish a simple HTML website on the internet using GitLab Pages.

GitLab Pages is a hosting service for static websites.

A static website consists of files such as:

- HTML.
- CSS.
- JavaScript.
- Images.

It does not run server-side application code such as PHP or ASP.

For example, you could publish:

- ESP32 course notes.
- Product documentation.
- Hardware setup instructions.
- A project portfolio.
- A static educational website.

### **Step D1 — Create a new project**

Create a blank GitLab project:

```
Project name: GitLab-Tutorial-Pages
Visibility: Public
Initialize with README: No
```

Clone the project:

```
cd C:\Users\YourUser\Documents

git clone https://gitlab.com/your-username/gitlab-tutorial-pages.git

cd gitlab-tutorial-pages
```

### **Step D2 — Create the website structure**

We will create a simple HTML website.

```
gitlab-tutorial-pages/
│
├── index.html
├── style.css
└── .gitlab-ci.yml
```

Notice that we do not need to create a `public` folder locally. GitLab's CI/CD job will create it.

### **Step D3 — Create index.html**

Create `index.html`:

```
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">

    <meta name="viewport"
          content="width=device-width, initial-scale=1.0">

    <title>My GitLab Website</title>

    <link rel="stylesheet" href="style.css">
</head>

<body>

    <header>
        <h1>Welcome to My Website</h1>
    </header>

    <main>
        <h2>My First GitLab Pages Project</h2>

        <p>
            This website is hosted using GitLab Pages.
        </p>

        <p>
            I created this website using HTML and CSS.
        </p>
    </main>

    <footer>
        <p>Published with GitLab Pages</p>
    </footer>

</body>
</html>
```

### **Step D4 — Create style.css**

```
body {
    font-family: Arial, sans-serif;
    margin: 0;
    background: #f4f4f4;
    color: #333;
}

header {
    background: #fc6d26;
    color: white;
    padding: 30px;
    text-align: center;
}

main {
    max-width: 800px;
    margin: 40px auto;
    background: white;
    padding: 30px;
    border-radius: 10px;
}

footer {
    text-align: center;
    padding: 20px;
}
```

### **Step D5 — Create the GitLab Pages pipeline**

Create `.gitlab-ci.yml`:

```

stages:
  - deploy

create-pages:
  stage: deploy
  script:
    - mkdir .public
    - cp -r * .public
    - mv .public public
  pages: true
  rules:
    - if: $CI_COMMIT_BRANCH == "main"

```

Let's understand what happens.

| Command or setting  | Purpose                                        |
| ------------------- | ---------------------------------------------- |
| `mkdir .public`     | Creates a temporary directory                  |
| `cp -r * .public`   | Copies the website files into it               |
| `mv .public public` | Renames the directory to `public`              |
| `pages: true`       | Marks the job as a GitLab Pages deployment job |
| `rules`             | Restricts this job to the `main` branch        |

GitLab Pages publishes the website files from the `public` directory.

This configuration uses the Pages job syntax documented by GitLab. In GitLab 17.10 and later, the default Pages publish directory is automatically included in job artifacts when not explicitly overridden.

Official reference: [Create a Pages website from scratch](https://docs.gitlab.com/user/project/pages/getting_started/pages_from_scratch/?utm_source=chatgpt.com).

### **Step D6 — Commit and push the website**

In PowerShell:

```
git add .

git commit -m "Create first GitLab Pages website"

git push -u origin main
```

The push triggers the deployment pipeline.

### **Step D7 — Check the pipeline**

1. Open your GitLab project.
2. Select **Build → Pipelines**.
3. Find the latest pipeline.
4. Open it.
5. Check the `create-pages` job.

Expected result:

```
Pipeline: Passed

Stage: deploy

Job: create-pages

Status: Passed
```

### **Step D8 — Find your website URL**

[【Gitlab CI/CD】005 Pages 部署静态网站](https://images.openai.com/static-rsc-4/nBl744xsNhAkyi9a1gDZbSNBF5A8ZuU6bRO1kvonOnk-e7NfN_hOlXNBGTmnOBlhokl8ISF7bC3nYsIaNg-YXopzZ3GucJqv64pO_Jdjg3lVJLRKlJxVC7YVeKpzW2zNFdytwpKmORmI-lGv96L2kDnrdo9RzZEbc1AEHnbaxqA?purpose=inline)

[blog.dkvirus.com](https://blog.dkvirus.com/gitlab/gitlab_pages/?utm_source=chatgpt.com)

1. Open the project.
2. Select **Deploy → Pages**.
3. Find the active deployment.
4. Select the website URL.

Your website will have a GitLab Pages URL.

The actual URL depends on your GitLab namespace, project path, and Pages domain configuration.

Open the URL in a browser.

You should see:

[Creating a simple website with HTML, CSS, and JavaScript for complete beginners](https://images.openai.com/static-rsc-4/RMhTP0fssYEs94WNCp3CdqgamxkLuAaEv14WZJx_YqhU5Kzwdwd3QmwuTV_u8J8jnDB7DXM6ZLibOl26zYxze8P4SQYWRojQgvuSlYA6oIVUKe0kdiWHOj9-R8hCqCKMF-qq3FGDZA2_YkYHCFVjJOySfYIOkOodb3E2x4DBI5A?purpose=inline)

[iq.opengenus.org](https://iq.opengenus.org/creating-a-simple-website-with-html-css-and-javascript/?utm_source=chatgpt.com)

Website deployed

### **Step D9 — Make a change and redeploy**

Modify `index.html`.

For example:

```
<h2>Welcome to ESP32 Learning Lab</h2>

<p>
    Learn embedded systems, IoT and software development.
</p>
```

Commit and push:

```
git add .

git commit -m "Update website content"

git push
```

GitLab automatically runs another pipeline.

After the deployment succeeds, refresh your website.

You should see your updated content.

### **Step D10 — Important GitLab Pages considerations**

- GitLab Pages supports static websites.
- Your Pages job needs a successful CI/CD deployment.
- The `public` directory must contain your website's entry point, normally `index.html`.
- The GitLab Pages feature must be enabled for the project.
- The project needs an available runner to execute the deployment job.
- A private project and public website are separate visibility decisions. Check Pages access control before publishing confidential content.
- A newly deployed site can take a little time to become accessible.

For more information:

- [GitLab Pages introduction](https://docs.gitlab.com/user/project/pages/?utm_source=chatgpt.com)
- [Pages settings](https://docs.gitlab.com/user/project/pages/introduction/?utm_source=chatgpt.com)
- [Pages troubleshooting](https://docs.gitlab.com/user/project/pages/?utm_source=chatgpt.com)

# **PART 5 — NEXT STEPS AND TROUBLESHOOTING**

## **5.1 Common GitLab errors and how to fix them**

The following table covers common problems you may encounter during the four tutorials.

| Error                           | Likely cause                                                               | Solution                                                                                                |
| ------------------------------- | -------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| Permission denied (publickey)   | SSH key missing, incorrect, or not loaded                                  | Check `ssh-add -l`, confirm your public key is registered, and test `ssh -T git@gitlab.com`             |
| HTTPS authentication failed     | Invalid password or PAT                                                    | Use a valid PAT with the required repository scope; check token expiry                                  |
| Pipeline failed                 | Incorrect YAML, script error, missing dependency, or runner problem        | Open the failed job and inspect its logs; correct the problem and push again                            |
| Merge conflict                  | Two branches changed overlapping content                                   | Pull the latest target branch, resolve conflict markers, commit, and push                               |
| Cannot push to protected branch | Your role or branch rules do not permit direct pushes                      | Create a feature branch and submit a Merge Request                                                      |
| GitLab Pages 404                | Incorrect output directory, missing `index.html`, or incomplete deployment | Check the Pages job, confirm the `public` directory and successful deployment, then check the Pages URL |

### **Troubleshooting 1 — SSH permission denied**

Check that your SSH key is loaded:

```
ssh-add -l
```

If the key is not listed:

```
ssh-add $env:USERPROFILE\.ssh\id_ed25519
```

Test GitLab:

```
ssh -T git@gitlab.com
```

If the connection still fails:

- Verify that the public key is in GitLab.
- Check that the private key matches the public key.
- Check whether your network blocks SSH connections.
- Try HTTPS as an alternative.

### **Troubleshooting 2 — Pipeline failed**

Open:

```
Project
  → Build
  → Pipelines
  → Failed pipeline
  → Failed job
```

Read the job log from the beginning.

Typical causes include:

- Incorrect `.gitlab-ci.yml` indentation.
- Invalid job name or configuration.
- Incorrect file paths.
- Missing packages.
- Failed tests.
- No eligible runner.

GitLab provides a CI Lint tool to validate pipeline configuration.

Open:

[GitLab CI Lint](https://docs.gitlab.com/ci/yaml/lint/?utm_source=chatgpt.com).

### **Troubleshooting 3 — Merge conflict**

First, switch to your feature branch:

```
git switch feature/my-feature
```

Retrieve the latest changes:

```
git fetch origin
```

Merge the latest main branch:

```
git merge origin/main
```

If Git reports a conflict, open the affected file.

You may see:

```
<<<<<<< HEAD

Your feature branch changes

=======

Changes from main

>>>>>>> origin/main
```

Edit the file to retain the correct code and remove the conflict markers.

Then:

```
git add .
```

```
git commit -m "Resolve merge conflict"
```

Push:

```
git push
```

Return to the Merge Request and check whether the conflict has been resolved.

### **Troubleshooting 4 — Cannot push to a protected branch**

For example:

```
You are not allowed to push code to protected branches.
```

This is usually an intentional security restriction.

Do not try to bypass it.

Instead:

```
git switch -c feature/my-change
```

Make your changes:

```
git add .
git commit -m "Implement feature"
git push -u origin feature/my-change
```

Then create a Merge Request targeting `main`.

If you are the project Maintainer, you can review the branch protection settings under:

**Settings → Repository → Branch rules.**

### **Troubleshooting 5 — GitLab Pages returns 404**

Check the following in order:

1. Did the pipeline succeed?
2. Did the Pages job execute?
3. Was a `public` directory generated?
4. Does `public` contain `index.html`?
5. Is GitLab Pages enabled?
6. Are you using the deployment URL displayed under Deploy → Pages?
7. Is Pages access control preventing your browser from viewing the website?

For our tutorial, the final job should create:

```
public/
    ├── index.html
    └── style.css
```

If the files are missing, inspect the `create-pages` job log.

Official reference: [GitLab Pages troubleshooting](https://docs.gitlab.com/user/project/pages/?utm_source=chatgpt.com).

## **5.2 A 30-minute GitLab practice plan**

The following exercise is designed to help a beginner understand the complete GitLab workflow in approximately 30 minutes.

**30-minute learning challenge**

Beginner

# **30 min**

One small project, one Issue, one Merge Request, and one pipeline.

1. **Minutes 0–5**

   5 min

   Create your GitLab account or sign in. Create a private project called `GitLab-30-Minute-Practice`.
2. **Minutes 5–10**

   5 min

   Clone the repository, create `README.md`, commit, and push your first change.
3. **Minutes 10–15**

   5 min

   Create an Issue called `Add project information`. Create a branch from the Issue.
4. **Minutes 15–20**

   5 min

   Modify a file, commit, push, and open a Merge Request. Review and merge.
5. **Minutes 20–30**

   10 min

   Create `.gitlab-ci.yml` with one simple job, push it, and verify that the pipeline succeeds.

At the end of the 30 minutes, you should understand the most important GitLab concepts:

- Repository.
- Project.
- Group.
- Issue.
- Branch.
- Commit.
- Merge Request.
- Pipeline.
- Job.
- Runner.

You do not need to master every GitLab feature immediately. Focus on understanding this workflow first.

## **5.3 Recommended next steps**

After completing the four tutorials, I recommend exploring these areas in the following order.

| Priority | Feature to learn      | What you should build                      |
| -------- | --------------------- | ------------------------------------------ |
| 1        | Branch protection     | Prevent direct pushes to `main`            |
| 2        | Merge Request reviews | Require developers to review changes       |
| 3        | CI/CD variables       | Store API keys and configuration securely  |
| 4        | Pipeline artifacts    | Save compiled binaries and test reports    |
| 5        | Package Registry      | Publish software and firmware packages     |
| 6        | Container Registry    | Store reusable Docker build environments   |
| 7        | Security scanning     | Check source code and dependencies         |
| 8        | GitLab Pages          | Publish technical documentation            |
| 9        | Pipeline schedules    | Automate nightly builds and testing        |
| 10       | GitLab API            | Automate project and repository operations |

### **A practical ESP32 development example**

Once you understand the basics, you can use GitLab to manage an ESP32 development project.

For example:

[Hello👋, welcome to ESP-Claw! | ESP-Claw Docs](https://images.openai.com/static-rsc-4/W-hAtMaBDYLonVQLeE1NdMsvDf6l7tUUt4HWhdBbAhc8IgJqhhBcf_LjhQ9Cv1oP3jXx4KPgLhm3I65l5OfnMlc4JuweGqPoFysYYbXT1a_4Iwwrmpj43JjgQ4oZ1hrQ80S8su4oF_gYNdSYulaWrG97Q90St6AIY4zcli-k3xM?purpose=inline)

[esp-claw.com](https://esp-claw.com/en/tutorial/?utm_source=chatgpt.com)

A possible project structure:

```
ESP32-Secure-OTA/
│
├── firmware/
│   ├── src/
│   │   └── main.cpp
│   └── include/
│       └── config.h
│
├── documentation/
│   ├── hardware.md
│   ├── ota-design.md
│   └── troubleshooting.md
│
├── tests/
│
├── .gitlab-ci.yml
│
└── README.md
```

You could implement the following workflow:

```
Developer updates ESP32 firmware
             |
             v
       GitLab Repository
             |
             v
       Merge Request
             |
             v
       CI/CD Pipeline
             |
       +-----+------+
       |            |
       v            v
    Compile      Unit Tests
       |            |
       +-----+------+
             |
             v
       Build Successful
             |
             v
       Generate firmware.bin
             |
             v
       Calculate SHA-256
             |
             v
       Publish to Package Registry
             |
             v
       ESP32 Secure OTA Download
```

This is a useful direction for a more advanced embedded development workflow.

Note that GitLab's generic pipeline example does not automatically compile Arduino or ESP-IDF firmware. You would need to configure the appropriate toolchain, build environment, and hardware-independent tests.

## **5.4 Official GitLab documentation — Recommended reading**

The following links take you directly to the relevant official GitLab documentation pages. They are organised by subject so you can continue learning without having to search through the entire documentation website.

### **A. Getting started and project management**

| Topic                      | Official documentation                                                                                        |
| -------------------------- | ------------------------------------------------------------------------------------------------------------- |
| GitLab user guide          | [Getting started with GitLab](https://docs.gitlab.com/user/?utm_source=chatgpt.com)                           |
| Create and manage projects | [Projects](https://docs.gitlab.com/user/project/?utm_source=chatgpt.com)                                      |
| Groups and namespaces      | [Namespaces](https://docs.gitlab.com/user/namespace/?utm_source=chatgpt.com)                                  |
| SSH authentication         | [SSH Keys](https://docs.gitlab.com/user/ssh/?utm_source=chatgpt.com)                                          |
| Personal Access Tokens     | [Personal Access Tokens](https://docs.gitlab.com/user/profile/personal_access_tokens/?utm_source=chatgpt.com) |

### **B. Planning and collaboration**

| Topic                 | Official documentation                                                                                           |
| --------------------- | ---------------------------------------------------------------------------------------------------------------- |
| Issues and work items | [Work Items](https://docs.gitlab.com/user/work_items/?utm_source=chatgpt.com)                                    |
| Issue Boards          | [Issue Boards](https://docs.gitlab.com/user/project/issue_board/?utm_source=chatgpt.com)                         |
| Merge Requests        | [Merge Requests](https://docs.gitlab.com/user/project/merge_requests/?utm_source=chatgpt.com)                    |
| Branch management     | [Branches](https://docs.gitlab.com/user/project/repository/branches/?utm_source=chatgpt.com)                     |
| Protected branches    | [Protected Branches](https://docs.gitlab.com/user/project/repository/branches/protected/?utm_source=chatgpt.com) |
| Code review approvals | [Merge Request Approvals](https://docs.gitlab.com/user/project/merge_requests/approvals/?utm_source=chatgpt.com) |

### **C. CI/CD and automation**

| Topic                | Official documentation                                                                       |
| -------------------- | -------------------------------------------------------------------------------------------- |
| CI/CD introduction   | [CI/CD](https://docs.gitlab.com/ci/?utm_source=chatgpt.com)                                  |
| YAML configuration   | [CI/CD YAML Reference](https://docs.gitlab.com/ci/yaml/?utm_source=chatgpt.com)              |
| Pipeline management  | [Pipelines](https://docs.gitlab.com/ci/pipelines/?utm_source=chatgpt.com)                    |
| Runner configuration | [Runners](https://docs.gitlab.com/ci/runners/?utm_source=chatgpt.com)                        |
| CI/CD variables      | [Variables](https://docs.gitlab.com/ci/variables/?utm_source=chatgpt.com)                    |
| Pipeline schedules   | [Pipeline Schedules](https://docs.gitlab.com/ci/pipelines/schedules/?utm_source=chatgpt.com) |
| Job artifacts        | [Job Artifacts](https://docs.gitlab.com/ci/jobs/job_artifacts/?utm_source=chatgpt.com)       |
| CI Lint              | [CI/CD YAML Validation](https://docs.gitlab.com/ci/yaml/lint/?utm_source=chatgpt.com)        |

### **D. Package management and deployment**

| Topic              | Official documentation                                                                                                          |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------- |
| Package Registry   | [Package Registry](https://docs.gitlab.com/user/packages/package_registry/?utm_source=chatgpt.com)                              |
| Generic Packages   | [Generic Packages](https://docs.gitlab.com/user/packages/generic_packages/?utm_source=chatgpt.com)                              |
| Container Registry | [Container Registry](https://docs.gitlab.com/user/packages/container_registry/?utm_source=chatgpt.com)                          |
| GitLab Pages       | [GitLab Pages](https://docs.gitlab.com/user/project/pages/?utm_source=chatgpt.com)                                              |
| Pages from scratch | [Create a Pages website](https://docs.gitlab.com/user/project/pages/getting_started/pages_from_scratch/?utm_source=chatgpt.com) |

### **E. Security and subscription information**

| Topic                   | Official documentation                                                                                               |
| ----------------------- | -------------------------------------------------------------------------------------------------------------------- |
| Application security    | [Application Security](https://docs.gitlab.com/user/application_security/?utm_source=chatgpt.com)                    |
| Dependency scanning     | [Dependency Scanning](https://docs.gitlab.com/user/application_security/dependency_scanning/?utm_source=chatgpt.com) |
| Secret detection        | [Secret Detection](https://docs.gitlab.com/user/application_security/secret_detection/?utm_source=chatgpt.com)       |
| Code quality            | [Code Quality](https://docs.gitlab.com/ci/testing/code_quality/?utm_source=chatgpt.com)                              |
| Free-tier limits        | [Free User and Group Limits](https://docs.gitlab.com/user/free_user_limit/?utm_source=chatgpt.com)                   |
| Storage limits          | [Storage Usage](https://docs.gitlab.com/user/storage_usage_quotas/?utm_source=chatgpt.com)                           |
| CI/CD compute limits    | [Compute Minutes](https://docs.gitlab.com/ci/pipelines/compute_minutes/?utm_source=chatgpt.com)                      |
| Subscription comparison | [GitLab Pricing](https://about.gitlab.com/pricing/?utm_source=chatgpt.com)                                           |

The Free-tier limits and feature availability described in this guide are based on GitLab's published documentation. Check the relevant pages before making purchasing or team-size decisions, as GitLab can change subscription limits and entitlements.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://docs.gitlab.com\&sz=32)

GitLab Docs+5



## **5.5 Final summary — What you should know after completing this guide**

## **Your GitLab learning journey**

1. **GitLab fundamentals**

   Understand repositories, projects, groups, namespaces, and visibility.
2. **Source code management**

   Clone repositories, create branches, commit changes, and push code.
3. **Team collaboration**

   Manage Issues, review Merge Requests, and protect important branches.
4. **CI/CD automation**

   Configure pipelines, stages, jobs, and runners.
5. **Software deployment**

   Publish websites using GitLab Pages and distribute packages through the Package Registry.
6. **DevSecOps**

   Integrate security scanning, code review, and automated software verification.

**My recommendation:** Start with Tutorials A, B, and C. These three will teach you the essential GitLab workflow. Then complete Tutorial D to understand how GitLab can automatically deploy a website.

For your ESP32 development work, the particularly interesting next step is combining GitLab's CI/CD pipeline with firmware compilation, SHA-256 verification, and the Generic Package Registry. This turns GitLab from a source-code repository into a practical firmware development and distribution platform.
