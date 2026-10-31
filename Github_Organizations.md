[![logo-color](https://img.shields.io/badge/Index-Installing_Git-086288?logo=github&logoColor=white&labelColor=5B7E3C)](#using-git-with-github-on-windows-and-macbook)
[![logo-color](https://img.shields.io/badge/Index-Organization_By_Meta-4D8DBA?logo=github&logoColor=white&labelColor=5B7E3C)](#esp32-course-distribution-with-github-organizations-meta-ai)
[![logo-color](https://img.shields.io/badge/Index-Classroom_50-97D7FF?logo=github&logoColor=white&labelColor=5B7E3C)](#classroom-50--complete-guide)
[![logo-color](https://img.shields.io/badge/Index-Github--Gists-B4E4D6?logo=github&logoColor=white&labelColor=5B7E3C)](#github-gists)
[![logo-color](https://img.shields.io/badge/Index-Github_Actions-218DAE?logo=github&logoColor=white&labelColor=5B7E3C)](#github-actions--introduction-features-and-step-by-step-setup)



# Secure ESP32 Course Distribution Using a GitHub Organization
**Practical implementation guide | Free-first | Arduino IDE and ESP-IDF | Student access control**

## 1. Executive recommendation

For your ESP32 course, I recommend using a **private GitHub Organization combined with GitHub Classroom, private repositories, and strict membership management**.

The objective is to let students access the course materials, clone repositories to their computers, modify code, and submit assignments without giving them permission to publish the instructor's materials publicly.

There is one important limitation: **GitHub can restrict access and discourage redistribution, but it cannot guarantee that a student who can read and download a file will not share it elsewhere.** Students can potentially copy files, take screenshots, or upload downloaded materials to another service. Your design should combine technical restrictions, clear policies, and prompt removal of access when a course ends.

### Recommended design

| Requirement | Recommended solution |
|---|---|
| Central course administration | GitHub Organization |
| Instructor materials | Private repositories |
| Student access to reference materials | Read-only team access |
| Student assignments | GitHub Classroom with private student repositories |
| Student submission | Private assignment repository and GitHub Classroom autograding, if required |
| Instructor and TA administration | Dedicated teams with least-privilege permissions |
| Prevent accidental public publication | Disable private-repository forking where supported, minimize administrative privileges, and review repository visibility |
| Prevent students from changing instructor materials | Give students read-only access to source repositories |
| Prevent unauthorized redistribution | Access restrictions, written student agreement, monitoring, and enforcement |
| Cost | Start with GitHub Free and verify eligibility for any Education benefits before purchasing anything |

**Key distinction:** Private repository access control is not the same as digital rights management. Students need to download course files to compile Arduino sketches and run ESP-IDF projects. Once downloaded, those files are outside GitHub's direct access controls.

---

## 2. Important platform status: GitHub Classroom

There is a significant date-sensitive issue with the requested design. GitHub's current documentation states that **GitHub Classroom is scheduled to be retired on August 28, 2026**. Today's date is October 30, 2026, so you should not build a new course around GitHub Classroom unless GitHub has announced a change to that retirement status. [GitHub Docs](https://docs.github.com/en/education/manage-coursework-with-github-classroom/teach-with-github-classroom/create-an-assignment-from-a-template-repository)


> [!TIP]
> Use Classroom 50 as altenative: https://github.com/foundation50/classroom50/wiki    
> Classroom 50 is a free, open-source tool for distributing and auto-grading programming assignments on GitHub. It's an alternative to GitHub Classroom, supported by the Fifty Foundation.


For this guide, I will design the core distribution system around private repositories and teams, then provide a Classroom workflow reference and a practical replacement based on private student repositories.

This preserves the important design principle: **instructor-controlled source repositories must remain separate from student working repositories**.

---

# Part A — Organization Setup

## 3. Create the GitHub Organization

### Step 1: Prepare your GitHub account

1. Create or use your existing personal account on GitHub.
2. Enable two-factor authentication (2FA).
3. Use a unique password and secure your account recovery methods.
4. Confirm that you can sign in and create private repositories.
5. If you teach through a school or polytechnic, check whether it already has a GitHub Organization or institutional policy for course repositories.

Official guide: [GitHub Organizations documentation](https://docs.github.com/en/organizations).

### Step 2: Create an Organization

1. Sign in to GitHub.
2. Open your profile menu.
3. Navigate to **Settings → Organizations** or the organization creation page.
4. Select the option to create a new organization.
5. Choose the free plan initially.
6. Select an organization name, such as:
   - `ESP32-Learning-Lab`
   - `ESP32-Course-2026`
   - `ESP32-Embedded-Systems`
7. Complete the setup.

Choose a name that will still make sense next year. I recommend a permanent organization name such as `ESP32-Learning-Lab`, with separate teams and repositories for each cohort.

**Do not create a separate organization for every assignment.** That creates unnecessary administrative work.

### Step 3: Configure the Organization

Open:

**Organization → Settings**

Configure the following settings, using the closest equivalent labels in the current GitHub interface.

| Setting | Recommended value | Reason |
|---|---|---|
| Organization base repository permission | None, if available | Prevents members from automatically receiving access to every repository |
| Public repository creation | Restrict to instructors/owners | Reduces accidental publication |
| Private repository creation | Restrict to instructors/owners initially | Centralizes repository creation |
| Private repository forking | Disabled | Reduces uncontrolled repository duplication |
| Member invitations | Restricted | Prevents unauthorized invitations |
| Member privileges | Minimize | Reduces accidental changes |
| Organization owners | One primary instructor and, ideally, one trusted backup | Reduces excessive privileges |
| Two-factor authentication | Require it for organization members where supported | Reduces account-compromise risk |

GitHub allows organization owners to configure base repository permissions, which apply to members across the organization's repositories. Setting base permission to `None` is a useful starting point because access can then be granted through specific teams. [GitHub](https://github.com/github/docs/blob/main/content/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles/setting-base-permissions-for-an-organization.md)

**Important:** Test the actual permissions using a separate student account before uploading sensitive instructor-only materials.

---

## 4. Enable GitHub Education Benefits

GitHub Education provides educational resources and benefits for eligible educators and institutions. However, do not assume that every benefit is automatically granted to every organization.

### Step-by-step application

1. Open the official [GitHub Education for Teachers application](https://docs.github.com/en/education/about-github-education/github-education-for-teachers/apply-to-github-education-as-a-teacher).
2. Sign in with your personal GitHub account.
3. Open your Education benefits settings.
4. Select **Start an application**.
5. Identify yourself as an educator.
6. Provide a verifiable school-issued email address.
7. Upload the requested evidence of your academic or teaching affiliation.
8. Submit the application.
9. Check the status of your application and any benefits made available after approval.

GitHub's stated requirements include educator or faculty status, a verifiable school-issued email address, supporting evidence of school affiliation, and a personal GitHub account. [GitHub Docs](https://docs.github.com/en/education/about-github-education/github-education-for-teachers/apply-to-github-education-as-a-teacher)

### What if you teach independently?

If you conduct private ESP32 training rather than teaching as an eligible employee of an accredited institution, do not assume you qualify for educator benefits.

Apply only with accurate information. You can still use GitHub's ordinary free functionality to build a private course repository system.

### Billing considerations

For your free-first design:

- Start with GitHub Free.
- Use private repositories, teams, and ordinary Git access where available.
- Avoid upgrading merely to obtain features you have not confirmed you need.
- Check the current plan requirements for every security control before depending on it.
- Do not enter payment details for optional services without reviewing their pricing and whether they are necessary.

**Budget recommendation: S$0 initially**, provided the current free-plan capabilities meet your needs and you do not require paid organizational security controls. Confirm current eligibility and feature limits before committing to this architecture.

---

# Part B — Access Architecture

## 5. Recommended repository structure

Separate the instructor's original teaching materials from student submissions. Organize repositories by purpose and use teams to grant access.

### Suggested architecture

```text
GitHub Organization: ESP32-Learning-Lab
│
├── Team: Instructors
│   └── Owner / Admin permissions as required
│
├── Team: TAs
│   └── Maintain or Write on designated repositories
│
├── Team: Cohort-2026
│   └── Read-only access to approved course materials
│
├── Team: Cohort-2027
│   └── Read-only access to approved course materials
│
├── Private Repository: esp32-course-materials
│   ├── 01-GPIO
│   ├── 02-WiFi-BLE
│   ├── 03-FreeRTOS
│   ├── 04-Sensors-IoT
│   ├── 05-ESP32-Memory-PSRAM
│   ├── 06-Secure-OTA
│   ├── schematics/
│   ├── diagrams/
│   └── PDFs/
│
├── Private Repository: esp32-code-examples
│   ├── Arduino/
│   ├── ESP-IDF/
│   └── PlatformIO/
│
├── Private Repository: esp32-instructor-only
│   ├── solutions/
│   ├── answer-keys/
│   ├── grading-rubrics/
│   └── instructor-notes/
│
├── Private Repository: esp32-assignment-01-template
│   ├── starter-code/
│   ├── README.md
│   └── tests/
│
├── Private Repository: assignment-01-studentA
├── Private Repository: assignment-01-studentB
├── Private Repository: assignment-01-studentC
│
└── Private Repository: esp32-admin
    └── Policies, roster procedures, and course administration
```

This structure is an example rather than a requirement to create every repository immediately.

### Why separate the repositories?

**Course materials:** Students can read and clone the reference materials, but cannot push changes to the instructor's source repository.

**Instructor-only materials:** Students receive no access. This repository contains solutions, assessment rubrics, and anything that must remain confidential.

**Assignment templates:** These contain starter code without exposing instructor solutions.

**Student repositories:** Each student has a separate workspace for experimentation and submission.

**Administration:** Keep administrative records and procedures separate from teaching materials. Do not put passwords, access tokens, student personal information, or other secrets into ordinary Git repositories.

### A practical simplification

For your first course, start with just four repositories:

1. `esp32-course-materials`
2. `esp32-instructor-only`
3. `esp32-assignment-01-template`
4. One private repository per student

Add more repositories only when a clear access-control or maintenance requirement justifies them.

---

## 6. Configure Teams

Teams are the main mechanism for granting access to groups of students without managing repository permissions individually.

Open:

**Organization → Teams → New team**

Create the following teams.

| Team | Members | Purpose |
|---|---|---|
| `Instructors` | Lead instructor and trusted backup | Manage course repositories and organization settings as needed |
| `TAs` | Teaching assistants | Help maintain teaching materials, review submissions, and support students |
| `Cohort-2026` | Students in the 2026 class | Read course materials |
| `Cohort-2027` | Students in the 2027 class | Read course materials |
| `ESP32-Project-Teams` | Students working together | Optional team-based projects |

### Assign repository permissions

Use the following as your initial permission matrix.

| Repository | Instructors | TAs | Cohort-2026 |
|---|---|---|---|
| `esp32-course-materials` | Admin | Read or Write, as needed | Read |
| `esp32-code-examples` | Admin | Read or Write, as needed | Read |
| `esp32-instructor-only` | Admin | Only if authorized | No access |
| `esp32-assignment-01-template` | Admin | Write | Read if students need to inspect it |
| `assignment-01-studentA` | Admin | Read or Write, as required | No access to other students' repositories |
| `assignment-01-studentB` | Admin | Read or Write, as required | No access to other students' repositories |

**Important:** A team should not automatically have access to every repository. Assign access deliberately.

For a small class, an instructor may choose to give TAs `Write` access only to the repositories they maintain. Use `Maintain` when a TA must manage repository settings or collaborators without needing full administrative control.

GitHub's repository roles include Read, Triage, Write, Maintain, and Admin, with different capabilities. ([GitHub repository roles](https://docs.github.com/en/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles))

**Steps to assign repository permissions:** [https://github.com/ooikk/Arduino-Documentation/blob/main/Github_Organizations.md#how-to-assign-repository-permissions-in-github-organization](https://github.com/ooikk/Arduino-Documentation/blob/main/Github_Organizations.md#how-to-assign-repository-permissions-in-github-organization)

---

## 7. Base Permissions, Custom Repository Roles, and Team Sync

### 7.1 Base permissions

**Recommended setting: `None`, where available.**

Base permissions apply to organization members across repositories. If you grant everyone `Read`, a student might gain read access to repositories you intended to keep restricted unless you configure additional protections.

With base permissions set to `None`, explicitly grant the cohort access to approved teaching repositories.

Note that base permissions do not apply to outside collaborators, and internal repositories have a minimum read-access level. ([GitHub documentation](https://github.com/github/docs/blob/main/content/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles/setting-base-permissions-for-an-organization.md))

### 7.2 Custom Repository Roles

GitHub supports custom repository roles on GitHub Enterprise Cloud.

They can be useful when predefined roles are too broad, but they are **not necessary for a typical free course setup**. Do not upgrade solely to obtain custom roles unless a specific permission requirement justifies the cost. [GitHub Docs](https://docs.github.com/en/enterprise-cloud%40latest/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles/managing-custom-repository-roles-for-an-organization)

For a free-first implementation, use the standard repository roles.

### 7.3 Team synchronization

Team synchronization can connect GitHub team membership to an external identity provider, such as an organization-managed identity directory, where supported.

For a small class, manual membership management is usually simpler.

Use team synchronization if your institution already has an identity-management system and your GitHub plan supports the required integration. Do not introduce it merely for a class of a few dozen students.

### 7.4 Organization members versus outside collaborators

This distinction matters.

**Organization member**

- Belongs to the organization.
- Can be managed through organization teams.
- Is subject to the organization's applicable membership and repository policies.

**Outside collaborator**

- Receives access to specific repositories without becoming an organization member.
- Is useful for a guest instructor, external examiner, or a contractor who needs access to only a few repositories.
- Is not automatically governed by member-only base permissions.

GitHub supports invitations by username or email, with the invited user required to accept the invitation. [GitHub Docs](https://docs.github.com/en/organizations/managing-membership-in-your-organization/inviting-users-to-join-your-organization?utm_source=chatgpt.com)

**Recommendation:** Use organization membership for regular students if your policies and plan support it. Use outside collaborators for narrowly scoped guest access, not as a workaround for missing security features.

---

# Part C — Protection Against Redistribution

## 8. Compare the available protection methods

The most important design decision is to distinguish between controls that prevent unauthorized access and controls that merely reduce opportunities for redistribution.

| Protection method | What it accomplishes | Free-first recommendation |
|---|---|---|
| Private repositories | Restricts access to authorized users | Essential |
| Read-only team permissions | Prevents students from modifying instructor repositories through normal GitHub permissions | Essential |
| Disable private-repository forking | Prevents supported GitHub forking workflows | Enable |
| Restrict repository creation | Reduces accidental public publication from the organization | Enable where available |
| Restrict repository visibility changes | Reduces the risk of an authorized user accidentally exposing a private repository | Enable where available |
| Branch protection | Controls how changes enter important branches | Useful for instructor repositories |
| Two-factor authentication | Reduces account takeover risk | Enable and require where supported |
| SAML SSO | Centralizes authentication through an identity provider | Usually unnecessary for a small free course |
| IP allow list | Restricts access from unauthorized IP addresses | Paid Enterprise feature; usually unsuitable for students |
| Email-domain restrictions | Limits certain organization email notifications to approved or verified domains | Paid Enterprise Cloud feature; not a general-purpose download restriction |
| Audit logs | Help investigate administrative and access-related changes | Review the features and retention available on your plan |
| End-of-semester access removal | Stops authorized accounts from accessing protected resources after removal | Essential |
| Student agreement | Establishes explicit obligations and potential consequences | Essential |

GitHub's security guidance recommends restricting repository forking and visibility changes, limiting repository creation, reviewing audit logs, and using appropriate branch protections. ([GitHub security guidance](https://docs.github.com/en/code-security/tutorials/secure-your-organization/prevent-data-leaks))

### 8.1 Disable forking of private repositories

This is one of the most useful controls for your course. GitHub allows organization owners to configure whether private repositories can be forked. New organizations are configured to disallow private-repository forking by default, according to GitHub's documentation. ([GitHub forking policy](https://docs.github.com/en/organizations/managing-organization-settings/managing-the-forking-policy-for-your-organization))

**Configuration steps:**

1. Open your organization.
2. Select **Settings**.
3. Navigate to **Member privileges**.
4. Find **Repository forking**.
5. Ensure that forking of private repositories is disabled.
6. Save the setting if a change is required.
7. Check your important repositories individually to confirm their effective settings.

**What this does not do:**

It does not prevent students from cloning a repository they can access. It also does not prevent copying files to another folder or creating a new repository containing those files.

For your ESP32 class, that limitation is acceptable: students need to clone the source code to compile and run it.

### 8.2 Disable unnecessary repository creation and visibility changes

Where supported, configure the organization to restrict:

- Creation of public repositories by ordinary members.
- Changing private repositories to public.
- Deleting or transferring important instructor repositories.

Keep these capabilities with trusted instructors or owners.

**Why this matters:** An accidental visibility change could expose all course files in a repository, including diagrams, PDFs, and code.

These controls reduce the likelihood of accidental disclosure. They do not prevent a student from publishing a separate copy of downloaded files.

### 8.3 Require SSO

SAML single sign-on can make sense when an educational institution has an identity provider such as Microsoft Entra ID or Okta.

However, GitHub's current documentation identifies SAML SSO as a GitHub Enterprise Cloud feature. Do not assume that ordinary GitHub Free includes it. ([GitHub SAML SSO documentation](https://docs.github.com/en/enterprise-cloud%40latest/organizations/managing-saml-single-sign-on-for-your-organization))

For a small course with students using personal GitHub accounts, use:

- Individual accounts.
- Two-factor authentication where supported and required.
- Controlled invitations.
- Explicit team assignments.
- Periodic membership reviews.

Do not build the course around SSO unless your institution already has the necessary plan and identity-provider infrastructure.

### 8.4 IP allow lists

GitHub Enterprise Cloud supports organization IP allow lists. They can restrict access to organization resources based on source IP addresses. ([GitHub IP allow list documentation](https://docs.github.com/en/enterprise-cloud%40latest/organizations/keeping-your-organization-secure/managing-security-settings-for-your-organization/managing-allowed-ip-addresses-for-your-organization))

For a student course, however, IP allow lists often create more problems than they solve:

- Students work from home, school, and mobile networks.
- Home IP addresses may change.
- A school network's public IP may not cover students working remotely.
- An IP restriction does not prevent copying files after a permitted download.

**Recommendation:** Do not pay for Enterprise solely to introduce an IP allow list for ordinary ESP32 teaching materials.

If you later teach a restricted corporate course, an IP allow list may be appropriate, provided students have a stable, approved network environment.

### 8.5 Limit invitations to email domains

Do not confuse domain verification with a complete student-email admission control.

GitHub's verified-domain feature establishes ownership of a domain. Restricting organization email notifications to approved or verified domains is a separate Enterprise Cloud capability. It does not generally stop someone with authorized repository access from cloning the repository. ([GitHub domain verification documentation](https://docs.github.com/en/organizations/managing-organization-settings/verifying-or-approving-a-domain-for-your-organization))

For a free-first class:

1. Obtain an official class roster.
2. Ask each student to provide their GitHub username.
3. Verify the username against the roster.
4. Invite only those accounts.
5. Keep a record of the cohort and its end date.
6. Review membership before each new semester.

If your institution requires strict institutional identity verification, ask its IT administrator about the available identity-management options.

---

## 9. Branch protection: protect the instructor's source of truth

Branch protection is primarily an **integrity control**, not a redistribution-prevention mechanism.

For example, you may want the `main` branch of `esp32-course-materials` to change only through reviewed pull requests.

### Suggested configuration

For an instructor-controlled source repository:

1. Open the repository.
2. Select **Settings → Branches**, or the equivalent ruleset configuration.
3. Create a rule or ruleset for `main`.
4. Require pull requests before merging, if your workflow requires them.
5. Require an appropriate review for changes.
6. Prevent force pushes and branch deletion where available.
7. Restrict bypass permissions to trusted maintainers when supported.

GitHub's documentation states that branch protection is available for public repositories under GitHub Free and for public and private repositories under paid plans such as GitHub Pro, Team, and Enterprise Cloud. Check the current plan requirements before applying protection to private instructor repositories. ([GitHub protected branches](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches))

**For your course:** Students should normally have `Read` access to instructor repositories, so they cannot push changes to those repositories in the first place. Branch protection is an additional safeguard for instructor and TA changes.

---

## 10. Audit logs and access monitoring

Audit logs help investigate who performed certain organization actions and when.

GitHub documents an organization audit log that records activities such as organization changes and repository-related events. Access and retention vary by plan and event type. ([GitHub audit log documentation](https://docs.github.com/en/organizations/keeping-your-organization-secure/managing-security-settings-for-your-organization/reviewing-the-audit-log-for-your-organization))

### Recommended review procedure

At the start of each semester:

- Review organization owners.
- Review all members and outside collaborators.
- Review team membership.
- Check the visibility of all course repositories.
- Check private-repository forking settings.
- Review relevant audit-log events.

At the end of each semester:

- Remove departing students.
- Remove temporary collaborators.
- Review any repository visibility changes.
- Check for unexpected repositories created in the organization.
- Investigate any known unauthorized publication or access incident.

**Important limitation:** Audit logs are not a comprehensive surveillance mechanism. Do not assume they record every file viewed, every local clone, or every attempt to copy material. GitHub's security documentation explicitly notes limitations in the available audit data. [GitHub Docs](https://docs.github.com/en/code-security/reference/security-incident-response/investigation-tools)

---

## 11. End-of-semester access revocation

This is one of the strongest practical controls available to a small course.

### Suggested procedure

1. Maintain a list of authorized students for each cohort.
2. Record the semester end date and any approved extension.
3. Export or record the current team membership before making changes.
4. Remove departing students from the organization or revoke their access to the course repositories.
5. Remove any outside collaborator permissions granted for temporary access.
6. Remove students from cohort teams.
7. Verify that the departed accounts can no longer access the protected repositories.
8. Keep instructor repositories private.
9. Review assignment repositories separately and decide whether they should be retained for grading, accreditation, or student records.

### Do not rely on removing a team alone

Students may have been granted access directly to a repository or through another team.

After removing someone from a cohort team, review the effective permissions on each protected repository. If the student is no longer authorized, remove every remaining route to access.

### What happens to downloaded files?

Removing a student from the organization does not delete files previously cloned to that student's computer.

This is why access revocation must be combined with a written agreement prohibiting redistribution and requiring the student to delete course materials when instructed, subject to any applicable retention obligations.

---

# Part D — Classroom and Assignment Workflow

## 12. GitHub Classroom: original workflow and current replacement

GitHub Classroom was designed to automate several tasks that otherwise require manual administration:

- Create a classroom.
- Prepare starter-code repositories.
- Generate a private repository for each student.
- Track accepted assignments.
- Provide feedback and, where configured, automatic grading.

However, GitHub's documentation states that Classroom was scheduled for retirement on August 28, 2026. Its documentation also describes migration options, including Classroom 50 and Codio. As of October 30, 2026, verify which replacement services are operational and what their current pricing and eligibility requirements are before adopting one. [GitHub Docs](https://docs.github.com/en/education/manage-coursework-with-github-classroom/get-started-with-github-classroom)

### 12.1 What the original Classroom workflow looked like

For reference, the original workflow was:

1. Create a private instructor template repository.
2. Populate it with starter code, documentation, and tests.
3. Open GitHub Classroom.
4. Create a classroom for the course.
5. Create an individual assignment.
6. Select the instructor template repository.
7. Choose private repository visibility.
8. Avoid granting students unnecessary repository administration privileges.
9. Publish the assignment invitation.
10. Students accept the assignment, and Classroom creates their individual repositories.
11. Students clone their repositories, work locally, and push their submissions.
12. The instructor reviews the submissions.

GitHub's documentation confirms that individual assignments could generate repositories from templates and that repository visibility and student administrative access were configurable. [GitHub Docs](https://docs.github.com/en/education/manage-coursework-with-github-classroom/teach-with-github-classroom/create-an-individual-assignment)

This workflow is useful as a reference when evaluating replacement platforms, but **do not assume that these Classroom functions remain available after the announced retirement**.

---

## 13. Recommended free-first replacement: private repositories and Git

For your ESP32 course, I recommend initially managing assignments directly through your GitHub Organization.

This requires more administration than an automated classroom service, but it is straightforward, transparent, and compatible with beginner-level Git workflows.

### Step 1: Create an assignment template

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/e471d2ca-d1c1-4f6e-a7fe-85f89d4aa1d7" />


Create a private repository named:

`esp32-assignment-01-template`

Example contents:

```text
esp32-assignment-01-template/
├── README.md
├── LICENSE
├── starter-code/
│   ├── Arduino/
│   │   └── assignment01.ino
│   └── ESP-IDF/
│       ├── CMakeLists.txt
│       └── main/
├── schematics/
├── diagrams/
└── tests/
```

Only include files students are authorized to receive.

Do not include:

- Instructor reference solutions.
- Hidden grading keys.
- Private credentials.
- Wi-Fi passwords or API tokens.
- Production signing keys.
- Internal assessment notes.

### Step 2: Create an individual private repository

For each student, create a repository such as:

- `assignment-01-studentA`
- `assignment-01-studentB`
- `assignment-01-studentC`

Use the private visibility setting.

Populate each repository from the assignment template. GitHub's web interface supports creating a repository from a template, where the applicable permissions and repository ownership allow it.

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/3e200c44-717e-4af6-905e-b9763ca84af0" />


Give the appropriate student write access to their repository. Give TAs access only when necessary.




**Do not grant students access to one another's individual repositories.**

### Step 3: Give students the repository URL

Provide each student with their assigned repository URL.

A beginner-friendly instruction sheet should tell them to:

1. Sign in to GitHub.
2. Open their assigned repository.
3. Copy its clone URL.
4. Open Git Bash or a terminal.
5. Run `git clone <repository-url>`.
6. Open the downloaded project in Arduino IDE or their ESP-IDF development environment.
7. Compile and test the project locally.
8. Commit changes.
9. Push their commits to GitHub.

For example:

```bash
git clone <repository-url>
cd <repository-folder>

git status
git add .
git commit -m "Complete assignment 1"
git push
```

These commands are suitable for a basic workflow, assuming Git is installed, the student has authenticated successfully, and the student's account has write access to the assigned repository.

**Steps to install Git:** https://github.com/ooikk/Arduino-Documentation/blob/main/Github_Organizations.md#using-git-with-github-on-windows-and-macbook

### Step 4: Review the submission

The instructor or TA can:

- Review the submitted source files.
- Examine the commit history.
- Leave feedback through issues or pull requests, if appropriate.
- Clone the repository to test the Arduino sketch or ESP-IDF project.
- Record the grade in the course's approved grading system.

For ESP32 assignments involving hardware, a successful GitHub submission does not prove that the code works on the actual board. Include a separate demonstration or test requirement where appropriate.

### Step 5: Automate later if necessary

If the class grows, consider a supported assignment-management platform that integrates with GitHub.

Evaluate replacements against these requirements:

| Requirement | Question to ask |
|---|---|
| Private student repositories | Can it create repositories owned by your Organization? |
| Template support | Can it populate repositories from instructor-controlled templates? |
| Student isolation | Can students be prevented from accessing other students' work? |
| Grading | Does it support manual feedback or automatic tests? |
| Cost | Is it genuinely free for your class size and institution? |
| Lifecycle management | Can access be revoked at the end of a semester? |
| Data migration | Can you export submissions, grades, and assignment records? |

Do not select a platform based solely on its name or its former GitHub Classroom compatibility. Verify its current availability and terms first.

---

# Part E — Policy and Legal Layer

## 14. Understand the difference between LICENSE and a student agreement

You requested three documents: a LICENSE file, a README disclaimer, and a student agreement.

They serve different purposes.

- **LICENSE:** States the permitted use of copyrighted materials and the rights granted to users.
- **README disclaimer:** Warns users about the course materials' confidentiality and redistribution restrictions.
- **Student agreement:** Records the obligations accepted by students, including prohibitions against public sharing.

A critical detail: **a standard open-source license is usually the wrong choice for this objective.** MIT, Apache-2.0, and similar licenses permit broad redistribution under their respective terms.

For confidential course materials, use a clearly worded, restrictive permission notice instead.

The templates below are starting points, not legal advice. If the materials belong to an educational institution or contain third-party content, confirm that you have the right to impose the proposed restrictions and have the appropriate institution or legal adviser approve the terms.

---

## 15. Template 1 — `LICENSE` file

Place the following in the root of each repository containing instructor-owned course materials.

The notice is intentionally restrictive. It is not an OSI-approved open-source license.

```markdown
# Restricted Educational Use License

Copyright (c) 2026 [Instructor Name or Rights-Holder]

All rights reserved.

## 1. Purpose

These materials are provided solely for use by students and authorized instructors participating in the ESP32 course identified by the repository owner.

The materials may include source code, Arduino sketches, ESP-IDF projects, Markdown documents, diagrams, schematics, photographs, presentations, PDFs, and other educational resources.

## 2. Limited Permission

Subject to the conditions below, an authorized student may:

- Access and download the materials for the authorized course.
- Clone the repository to a personal computer for coursework.
- Compile, execute, test, and modify code for educational exercises.
- Make private working copies as reasonably necessary to complete course activities.
- Submit assignments through the designated course submission process.

This permission is limited to the educational purposes stated above.

## 3. Prohibited Activities

Unless the rights-holder gives prior written permission, the recipient must not:

- Publish the materials, or substantial portions of them, in a public repository.
- Upload the materials to public websites, file-sharing services, forums, or social media.
- Distribute the materials to unauthorized persons.
- Share repository access with unauthorized persons.
- Sell, sublicense, or commercially redistribute the materials.
- Remove copyright notices or misrepresent the materials as their own original work.
- Use the materials in a manner that violates applicable law or the recipient's course obligations.

## 4. Confidentiality and Access

Access to a private repository does not authorize redistribution outside the course.

Repository access may be revoked when the course ends or when authorization is otherwise withdrawn, subject to applicable institutional policies and legal obligations.

## 5. Ownership

Except for third-party materials identified separately, the rights-holder retains the applicable rights in the course materials.

Nothing in this notice transfers ownership of the materials to the recipient.

Third-party components remain subject to their own licenses and terms.

## 6. No Warranty

The materials are provided for educational purposes without a warranty of any kind, to the extent permitted by applicable law.

The recipient is responsible for testing code and verifying hardware safety before use.

## 7. Permissions and Questions

Requests for permission to reuse or redistribute materials outside the authorized course must be directed to:

[Instructor or Rights-Holder Contact]

This notice is subject to applicable law and any overriding institutional policies or agreements.
```

### Important implementation notes

1. Replace the placeholder rights-holder with the actual owner of the materials.
2. Use the correct copyright year for the relevant material.
3. Do not claim ownership of third-party libraries, manufacturer documentation, or other content you do not own.
4. Check that your institution permits you to impose these conditions.
5. If students contribute code to the course repository, establish the ownership and licensing arrangements for those contributions separately.

A LICENSE file does not prevent copying technically. It helps communicate the conditions under which the materials are supplied.

---

## 16. Template 2 — README disclaimer

Place this notice near the beginning of each course-materials repository's `README.md`.

```markdown
# ESP32 Course Materials — Authorized Use Only

Welcome to the ESP32 course repository.

This private repository contains educational materials supplied for authorized course participants. Its contents may include Arduino sketches, ESP-IDF projects, source code, schematics, diagrams, Markdown notes, and PDF documents.

## Permitted Use

Authorized students may:

- Read and clone the repository.
- Download the materials for course-related work.
- Modify and test code for their own coursework.
- Submit assignments using the designated submission process.

## Redistribution Restrictions

**You must not publish, upload, or distribute these course materials outside the authorized class without prior written permission from the rights-holder.**

In particular, do not:

- Make the materials public on GitHub or another code-hosting platform.
- Upload course PDFs, diagrams, or source files to public websites or file-sharing services.
- Share your repository access with unauthorized persons.
- Copy substantial portions of the instructor's materials into public repositories.
- Redistribute the materials commercially.

Your access to a private GitHub repository does not grant permission to publish its contents elsewhere.

## Student Assignments

Submit your work only through the repository or other submission channel assigned by the instructor.

Do not include passwords, Wi-Fi credentials, API tokens, private keys, or other secrets in your commits.

## End of Course

When your course access ends, you must follow the instructor's instructions regarding continued access, retention, and deletion of course materials, subject to applicable legal or institutional retention requirements.

## Questions and Permission Requests

Contact the instructor if you need permission to reuse or redistribute any material.

By accessing this repository, you acknowledge that the materials are subject to the applicable course policies and usage terms.
```

**Recommendation:** Put the full policy in `LICENSE` and the short notice in `README.md`. Repeat the notice in assignment repositories so that students encounter it even when working in a separate repository.

---

## 17. Template 3 — Student agreement

This agreement should be provided before students receive access to the private repositories.

For an institutional course, have the institution approve the agreement and determine how student acceptance should be recorded. For a private training course, adapt it to your enrollment terms.

```markdown
# ESP32 Course Materials — Student Access and Use Agreement

**Course:** ESP32 Embedded Systems and IoT  
**Cohort:** [Cohort / Semester / Year]  
**Instructor or Institution:** [Name of Rights-Holder]

**Student Name:** __________________________________

**GitHub Username:** _______________________________

**Date:** __________________________________________

## 1. Purpose of Access

I understand that I am being granted access to private GitHub repositories containing educational materials for the ESP32 course.

These materials may include source code, Arduino sketches, ESP-IDF projects, schematics, diagrams, PDFs, presentations, technical notes, sample projects, and assessment resources.

My access is provided for the purpose of participating in the authorized course and completing its educational activities.

## 2. Permitted Activities

I may:

- Read and clone the authorized repositories.
- Download course materials to my computer for learning and coursework.
- Compile, test, and modify code for authorized exercises.
- Submit my own work using the designated submission process.
- Retain copies of my own work as permitted by the course and applicable institutional policies.

## 3. Prohibition Against Unauthorized Redistribution

I agree that I will not, without prior written permission from the rights-holder:

- Publish course materials in a public GitHub repository or another public code-hosting service.
- Upload instructor-provided PDFs, diagrams, schematics, source code, or notes to public websites, file-sharing platforms, or social media.
- Distribute the materials to people who are not authorized course participants.
- Share my private repository access or credentials with another person.
- Sell, sublicense, or commercially redistribute the instructor's course materials.
- Remove copyright or ownership notices, or falsely represent the instructor's work as my own.

These restrictions apply to instructor-provided materials and other content that I am not independently authorized to redistribute. They do not transfer ownership of my original work to the instructor.

## 4. Responsible Use of GitHub

I agree to:

- Keep my GitHub account credentials secure.
- Use only the repositories assigned or authorized for me.
- Avoid attempting to access other students' private repositories without permission.
- Avoid committing passwords, API tokens, Wi-Fi credentials, private keys, or other secrets.
- Follow the instructor's repository access and assignment submission instructions.

## 5. Confidentiality and End-of-Course Access

I understand that private repository access is granted for a limited educational purpose.

When my course access ends, I agree to stop accessing the course repositories if my authorization has been withdrawn and to follow reasonable, lawful instructions concerning the retention or deletion of instructor-provided materials.

Any retention obligations imposed by applicable law or institutional policy take precedence where required.

## 6. Reporting Unauthorized Disclosure

If I become aware that course materials have been published or distributed without authorization, I will notify the instructor or the designated institutional contact.

## 7. Ownership and Third-Party Materials

I understand that the instructor, institution, or other rights-holders retain the applicable rights to their materials.

Third-party software, libraries, and documentation remain subject to their respective licenses and terms.

Nothing in this agreement transfers ownership of the instructor's materials to me or automatically transfers ownership of my original work to the instructor.

## 8. Consequences of Breach

I understand that a breach of this agreement may result in suspension or removal of my repository access and referral to the institution or other appropriate rights-holder for further action, subject to applicable policies and law.

Any additional consequences must be imposed in accordance with the applicable enrollment terms, institutional rules, and legal requirements.

## 9. Acknowledgment

By signing below, I acknowledge that I have read and understood this agreement and agree to comply with its applicable terms.

**Student Signature:** ______________________________

**Date:** __________________________________________

**Instructor or Authorized Representative:** ______________________________

**Date:** __________________________________________
```

### Practical recommendations for using the agreement

- Collect acceptance before adding students to the course organization.
- Keep a record of the accepted version and the date of acceptance.
- Use the same version for all students in a cohort unless an approved amendment is necessary.
- Obtain parental or guardian involvement where required by applicable law or institutional policy.
- Do not claim that signing the agreement guarantees that redistribution will never occur.

For an online-only course, you could use an institutional enrollment system or an approved electronic-signature process rather than collecting paper forms.

---

# Part F — Recommended Implementation Plan

## 18. Roll out the system in three phases

### Phase 1: Minimum viable secure setup

**Goal: Protect the source materials without introducing unnecessary complexity or cost.**

1. Create the GitHub Organization.
2. Enable 2FA on the instructor account.
3. Set base permissions to `None` where available.
4. Restrict public repository creation and visibility changes where supported.
5. Disable private-repository forking.
6. Create the `Instructors`, `TAs`, and `Cohort-2026` teams.
7. Create private repositories for course materials and instructor-only materials.
8. Give the cohort `Read` access to the approved teaching repositories.
9. Create a private assignment template.
10. Prepare the LICENSE, README disclaimer, and student agreement.

**Expected result:** Students can read and clone approved materials but cannot push changes to the instructor's repositories.

### Phase 2: Student onboarding and submissions

**Goal: Give every student a separate working environment.**

1. Collect GitHub usernames from the official class roster.
2. Obtain agreement acceptance.
3. Invite the verified accounts.
4. Add students to the appropriate cohort team.
5. Create one private repository per student and assignment.
6. Grant each student write access only to their own assignment repository.
7. Test the process using a separate student account.
8. Give students a short Git tutorial.
9. Explain how to clone, commit, push, and submit their work.
10. Document the end-of-semester access removal procedure.

### Phase 3: Review and improve

**Goal: Maintain security throughout the course.**

At regular intervals:

- Review membership and team assignments.
- Check the visibility of important repositories.
- Review relevant audit events.
- Remove obsolete collaborators.
- Check that instructor-only repositories remain inaccessible to students.
- Investigate any reported unauthorized redistribution.
- Keep a backup of the instructor's source materials.

If your course expands, investigate supported assignment-management services and automation rather than immediately purchasing Enterprise features.

---

# Part G — Instructor Checklist

## 19. Before the semester starts

### Organization and account security

- [ ] Create the GitHub Organization.
- [ ] Secure instructor accounts with 2FA.
- [ ] Limit organization owners to trusted administrators.
- [ ] Set the lowest practical base repository permission.
- [ ] Restrict repository creation and visibility changes where supported.
- [ ] Disable private-repository forking.
- [ ] Review the current plan's feature and billing limitations.

### Repositories and permissions

- [ ] Create `esp32-course-materials`.
- [ ] Create `esp32-code-examples`.
- [ ] Create `esp32-instructor-only`.
- [ ] Create the assignment template repository.
- [ ] Keep solutions and grading keys separate.
- [ ] Create the `Instructors`, `TAs`, and cohort teams.
- [ ] Give students read-only access to instructor materials.
- [ ] Give students write access only to their authorized submission repositories.
- [ ] Confirm that students cannot access other students' private assignments.

### Student onboarding

- [ ] Verify GitHub usernames against the class roster.
- [ ] Obtain the required student agreement acceptance.
- [ ] Invite only authorized students.
- [ ] Provide the Git clone and submission instructions.
- [ ] Test the complete workflow with a student account.
- [ ] Confirm that no repository contains credentials or secrets.

## 20. During the semester

- [ ] Review membership when students join or leave.
- [ ] Check that teaching repositories remain private.
- [ ] Review repository permissions when adding TAs.
- [ ] Keep the instructor's source repository as the authoritative version.
- [ ] Review submissions and commit history.
- [ ] Keep grading information separate from general course materials.
- [ ] Investigate reported unauthorized disclosures.
- [ ] Maintain backups of the original teaching materials.

## 21. At the end of the semester

- [ ] Export or record required submissions and grades.
- [ ] Remove departing students from the organization or revoke their repository access.
- [ ] Remove outside collaborators who no longer require access.
- [ ] Check for direct repository permissions that bypass cohort teams.
- [ ] Review relevant audit events.
- [ ] Verify that instructor-only repositories remain private.
- [ ] Retain student records in accordance with applicable policies.
- [ ] Follow up on any known unauthorized redistribution.
- [ ] Prepare the next cohort without reusing obsolete access permissions.

---

# Final Recommendation

For your ESP32 course, I would implement the following design first:

| Component | Final choice |
|---|---|
| Hosting | GitHub Organization |
| Cost | GitHub Free initially |
| Course notes, PDFs, diagrams, and example code | Private repositories |
| Instructor solutions | Separate private repository |
| Student access | Read-only team access |
| Student assignments | Individual private repositories |
| Repository forking | Disabled for private repositories |
| Organization base permissions | `None`, where available |
| Account security | 2FA and least-privilege access |
| Redistribution policy | Restrictive LICENSE notice, README disclaimer, and signed or otherwise properly accepted student agreement |
| End-of-semester security | Revoke repository access and verify effective permissions |
| Assignment automation | Evaluate currently supported alternatives to retired GitHub Classroom |

### The most important lesson

**Protect the original repository, isolate student work, and revoke access promptly.**

Do not spend money on Enterprise features before you have established these fundamentals.

The remaining limitation is unavoidable in a download-based teaching model: a student who is authorized to clone an Arduino or ESP-IDF project can make a copy and potentially redistribute it. Private repositories, disabled forking, and written agreements reduce the risk, but they cannot guarantee that the materials will never be copied.

For an ordinary ESP32 course, this is a practical balance between security, student usability, and cost.

---

# Using Git with GitHub on Windows and MacBook

Since you access GitHub through a browser, you can use Windows PowerShell on your Windows computer or Terminal on your MacBook to work with GitHub repositories.

The Git commands are almost identical on both systems. The main difference is how you open the terminal and install Git.

## 1. Windows — PowerShell

### Step 1: Install Git

If Git is not installed, download and install Git for Windows from the official website.

[Git for Windows](https://git-scm.com/install/windows)

After installation:

1. Open the Windows Start menu.
2. Search for PowerShell.
3. Open Windows PowerShell.
4. Check that Git is installed:

```powershell
git --version
```

You should see a version number, such as `git version 2.x.x`.

### Step 2: Clone your GitHub repository

1. Open your private assignment repository in your browser.
2. Click the green **Code** button.
3. Select **HTTPS**.
4. Copy the repository URL.
5. Open PowerShell.
6. Run:

```powershell
# Navigate to the folder where you want to store your assignments
cd "$HOME\Documents"

# Clone your GitHub repository
git clone <repository-url>

# Enter the repository folder
cd <repository-folder>
```

Replace `<repository-url>` with the URL you copied from GitHub.

For example, if your repository is named `assignment-01-studentA`:

```powershell
git clone https://github.com/ESP32-Learning-Lab/assignment-01-studentA.git

cd assignment-01-studentA
```

Use your actual organization name and repository URL.

### Step 3: Edit your assignment

You can edit your Arduino `.ino` files using Arduino IDE, or your ESP-IDF project using your chosen development environment.

After saving your changes, return to PowerShell.

### Step 4: Submit your assignment

```powershell
# Check which files have changed
git status

# Stage your changes
git add .

# Commit your changes
git commit -m "Complete assignment 1"

# Upload your changes to GitHub
git push
```

After `git push` succeeds, refresh your repository page in the browser. Your changes should appear on GitHub.

## 2. MacBook — Terminal

### Step 1: Check whether Git is installed

1. Open Spotlight Search by pressing `Command + Space`.
2. Type `Terminal`.
3. Open Terminal.
4. Run:

```bash
git --version
```

If Git is not installed, macOS may prompt you to install the Xcode Command Line Tools. Follow the prompt.

Alternatively, install Git using the official instructions:

[Git installation instructions](https://git-scm.com/install/mac)

### Step 2: Clone your GitHub repository

In Terminal, run:

```bash
# Navigate to the Documents folder
cd "$HOME/Documents"

# Clone your GitHub repository
git clone <repository-url>

# Enter the repository folder
cd <repository-folder>
```

For example:

```bash
git clone https://github.com/ESP32-Learning-Lab/assignment-01-studentA.git

cd assignment-01-studentA
```

Again, replace the example URL with your actual repository URL.

### Step 3: Edit your assignment

Open your Arduino sketch in Arduino IDE or your ESP-IDF project in your preferred development environment.

Make your changes and save the files.

### Step 4: Submit your assignment

```bash
# Check which files have changed
git status

# Stage your changes
git add .

# Commit your changes
git commit -m "Complete assignment 1"

# Upload your changes to GitHub
git push
```

Refresh the repository page in your browser to verify the submission.

## 3. Windows vs. MacBook — Command comparison

| Operation | Windows PowerShell | MacBook Terminal |
|---|---|---|
| Check Git installation | `git --version` | `git --version` |
| Navigate to Documents | `cd "$HOME\Documents"` | `cd "$HOME/Documents"` |
| Clone repository | `git clone <repository-url>` | `git clone <repository-url>` |
| Enter repository | `cd <repository-folder>` | `cd <repository-folder>` |
| Check changes | `git status` | `git status` |
| Stage changes | `git add .` | `git add .` |
| Commit changes | `git commit -m "Complete assignment 1"` | `git commit -m "Complete assignment 1"` |
| Upload changes | `git push` | `git push` |

The Git commands themselves are the same on both platforms.

## 4. Important: First-time GitHub authentication

Cloning a private repository and uploading changes require authentication.

For private repositories, GitHub may prompt you to authenticate through a browser or a supported credential manager.

If Git asks for a GitHub password in the terminal, do not enter your normal GitHub account password. GitHub does not accept account passwords for Git operations over HTTPS.

Use a supported authentication method, such as Git Credential Manager or GitHub CLI. See the official instructions:

[GitHub authentication for Git](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/about-authentication-to-github)

For beginners, I recommend using HTTPS with browser-based authentication where available.

### Two important rules for your students

1. **Clone only once.** The first `git clone` downloads the repository. For subsequent sessions, navigate to the existing repository folder instead of cloning it again.
2. **Check `git status` before committing.** This helps students identify which files they are about to submit.

For example, when returning to an existing assignment on Windows:

```powershell
cd "$HOME\Documents\assignment-01-studentA"

git status
git add .
git commit -m "Update assignment 1"
git push
```

The equivalent commands work on a MacBook, using the same folder path syntax shown above.

**Security reminder:** Students should push work only to their assigned private repositories. They should never put GitHub access tokens, Wi-Fi passwords, or private keys into their Arduino or ESP-IDF source files.

---

# How to Assign Repository Permissions in GitHub Organization

Since you use GitHub through a web browser, you can configure all repository permissions through GitHub's website. You do not need PowerShell, Git commands, or the GitHub CLI to manage permissions.

For your ESP32 course, the goal is to let students read and clone your teaching materials but prevent them from modifying your original course repositories.

## 1. Understand the five repository roles

GitHub provides five standard repository roles.

| Role | What the user can generally do | Recommended for |
|---|---|---|
| Read | View, clone, and download repository contents | Students accessing course materials |
| Triage | Read and manage issues and pull requests without write access to repository contents | TAs helping manage student issues |
| Write | Push code and make changes to the repository | Students submitting assignments to their own repositories |
| Maintain | Manage many repository tasks without full administrative control | TAs maintaining selected repositories |
| Admin | Manage repository settings, permissions, and other administrative functions | Trusted instructors |

For your course, remember these two rules:

- Students should normally have `Read` access to the instructor's course-materials repositories.
- Students should have `Write` access to their own assignment repositories, but not to other students' repositories.

GitHub's official guide: [Managing repository roles](https://docs.github.com/en/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles)

## 2. Method A — Assign permissions using Teams (recommended)

This is the best method for your ESP32 course because you can assign permissions to an entire cohort instead of adding students individually to every repository.

### Step 1: Open your repository

1. Sign in to [GitHub](https://github.com/).
2. Open your ESP32 Organization.
3. Click **Repositories**.
4. Select your course repository, for example `esp32-course-materials`.

### Step 2: Open repository access settings

1. Click **Settings** in the repository's top menu.
2. In the left sidebar, find the **Access** section.
3. Click **Collaborators & teams**.

GitHub displays the people and teams that currently have access to the repository. [GitHub Docs](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/managing-repository-settings/managing-teams-and-people-with-access-to-your-repository)

### Step 3: Add a team

1. Under **Manage access**, click **Add people** or **Add teams**. The label may vary slightly.
2. Search for your team, such as `Cohort-2026`.
3. Select the team.
4. Under **Choose a role**, select `Read`.
5. Click **Add team to this repository** or the equivalent confirmation button.

Your students now have read access through that team, provided they are members of it and have accepted any required invitations.

### Step 4: Repeat for the other teams

For `esp32-course-materials`, you might configure:

| Team | Permission |
|---|---|
| `Instructors` | `Admin`, for trusted repository administrators |
| `TAs` | `Read` or `Write`, depending on responsibilities |
| `Cohort-2026` | `Read` |

For `esp32-instructor-only`:

| Team | Permission |
|---|---|
| `Instructors` | `Admin` |
| `TAs` | `Read`, if authorized |
| `Cohort-2026` | No access |

**Important:** Do not add the student cohort to your instructor-only repository.

Official instructions: [Managing team access to an organization repository](https://github.com/github/docs/blob/main/content/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles/managing-team-access-to-an-organization-repository.md)

## 3. Method B — Assign permissions to an individual student

Use this method when a student needs access to a specific assignment repository.

For example, suppose you have created:

```text
assignment-01-studentA
```

You want Student A to submit their code but prevent them from changing the original teaching materials.

### Step 1: Open the student's repository

1. Open your GitHub Organization.
2. Select `assignment-01-studentA`.
3. Click **Settings**.
4. Under **Access**, click **Collaborators & teams**.

### Step 2: Add the student

1. Click **Add people**.
2. Search for the student's GitHub username.
3. Select the correct account.
4. Choose the `Write` role.
5. Confirm the invitation.

The student must accept the invitation if GitHub requires it.

### Step 3: Verify the permission

1. Return to **Settings → Collaborators & teams**.
2. Confirm that the student appears with the `Write` role.

Repeat for other students, giving each student access only to their assigned repository.

Official instructions: [Managing teams and people with access to your repository](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/managing-repository-settings/managing-teams-and-people-with-access-to-your-repository)

## 4. Recommended permission matrix for your course

Here is the configuration I recommend implementing first.

| Repository | Instructors | TAs | Cohort-2026 | Individual student |
|---|---|---|---|---|
| `esp32-course-materials` | Admin | Read | Read | No additional access needed |
| `esp32-code-examples` | Admin | Read | Read | No additional access needed |
| `esp32-instructor-only` | Admin | As authorized | No access | No access |
| `esp32-assignment-01-template` | Admin | Write | Read, if needed | Read, if needed |
| `assignment-01-studentA` | Admin | Read or Write | No cohort-wide access | Student A: Write |
| `assignment-01-studentB` | Admin | Read or Write | No cohort-wide access | Student B: Write |

The table describes the intended permissions. Check for inherited permissions, organization-wide access, and any direct grants before considering the configuration complete.

### A critical security check

Suppose you grant Student A `Write` access to their assignment repository but also add them to a team with `Write` access to every assignment repository.

Student A could then modify other students' submissions.

Avoid granting cohort-wide `Write` access to student assignment repositories. Give each student access only to their own repository.

## 5. How to check who has access

For each important repository:

1. Open the repository.
2. Click **Settings**.
3. Select **Collaborators & teams**.
4. Review the list under **Manage access**.
5. Check the role assigned to every team and individual.
6. Remove any unauthorized access.

If GitHub shows **Mixed roles** next to a person, investigate the conflicting permissions. A user may have access through multiple routes. [GitHub Docs](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/managing-repository-settings/managing-teams-and-people-with-access-to-your-repository)

For your instructor-only repository, verify that the student cohort has no access.

## 6. How to remove a student's access

When a student leaves the course:

1. Open each relevant repository.
2. Go to **Settings → Collaborators & teams**.
3. Find the student's direct access, if any.
4. Click **Remove**.
5. Review the student's team memberships and remove any that are no longer appropriate.
6. Verify that no other team or direct permission still grants access.

Removing a student's repository access does not delete copies previously cloned to their computer. [GitHub Docs](https://docs.github.com/en/enterprise-cloud%40latest/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles/managing-team-access-to-an-organization-repository)

## 7. My recommendation: configure one repository first

Before configuring all your ESP32 repositories, test the process with just these two:

### Repository A: `esp32-course-materials`

- `Instructors`: Admin
- `TAs`: Read
- `Cohort-2026`: Read

### Repository B: `assignment-01-studentA`

- `Instructors`: Admin
- `Student A`: Write
- Other students: No access

Test using a separate student account. Confirm that the student can clone the course materials, cannot push changes to the instructor's repository, and can push changes to their own assignment repository.

Once these tests pass, repeat the configuration for the remaining repositories.

One question before you proceed: Have you already created the `Instructors`, `TAs`, and `Cohort-2026` teams in your GitHub Organization, or do you need instructions to create those teams first?

---

# ESP32 Course Distribution with GitHub Organizations (Meta AI)

As an ESP32 instructor, your goal isn't DRM - it's **friction**. You want students to have Read/Clone but not be able to mass-distribute. GitHub Organizations does this well if you set it up right.

Here is the complete system I recommend for free-tier instructors.

## 1. SETUP - Create and Harden the Organization

### Step 1: Create the Org

1. GitHub > Top right + > New Organization > Free plan
2. Name: `esp32-course-2026` or `your-university-esp32`. Do NOT use your personal account as the classroom.
3. Billing: For education you get unlimited private repos on Free. Apply for **GitHub Education** at education.github.com - benefits include GitHub Team free for your Org, and Copilot for students.

### Step 2: Lock down Org Settings

Go to Org > Settings:

**Member privileges:**

- Base permissions: `No permission` - This is critical. New members get nothing by default until you put them in a Team.
- Repository creation: `No one` or `Only owners` - Prevents a student accidentally creating a public repo with your materials inside.
- Allow members to create public repos: OFF
- Allow forking of private repositories: OFF. By default, new organizations are configured to disallow forking of private repositories.

**Security:**

- Two-factor authentication: Require for all members
- If you have Enterprise Cloud via Education: Enable SAML SSO + restrict to `@youruniversity.edu` email domain

## 2. ACCESS ARCHITECTURE

Don't add students directly to repos. Always add via Teams.

### Recommended Teams

- `Instructors` - Owner role in Org
- `TAs` - Maintainer on all course repos
- `Cohort-2026-S2` - Students for this semester
- `Cohort-2026-S2-Group-A` - Optional lab groups

### Member vs Outside Collaborator

Use **Members** for TAs/instructors, **Outside Collaborators** is not needed. For students, invite them as Members of the Org but with `No permission` base, so their only access comes from the `Cohort` Team. When semester ends, remove them from the Team = access revoked.

### Repository Types

- **Private:** Use this for EVERYTHING. A private repository is only visible to owner and explicitly invited collaborators. This is your main protection.
- **Internal:** Do NOT use. Internal is visible to all enterprise members.
- **Public:** Only for your course syllabus/landing page.

### Recommended Repo Structure

```text
esp32-course-2026 (Organization)
├── 00-course-info (Private) -> Syllabus, schedule, policies - Read for all
├── 01-lecture-notes (Private) -> Markdown + PDFs - Read-only for students
│   ├── week01-gpio/
│   ├── week02-wifi/
│   └── schematics/
├── 02-lab-templates (Private, marked as Template Repositories)
│   ├── lab01-blink-template
│   ├── lab02-ble-scan-template
│   └── final-project-template
├── 03-references (Private) -> Datasheet, libs
└── (Student repos auto-created by Classroom, not listed here)
    ├── lab01-blink-student123
    └── lab02-wifi-student123
```

### Fine-Grained Permissions

1. **Team Access:** Org > Teams > `Cohort-2026-S2` > Repositories > Add `01-lecture-notes` with `Read` role. Students can clone/pull but cannot push.
2. **Custom Repository Roles:** If on Team/Enterprise, you can create more granular control. You can more granularly control access with custom repository roles. You can create up to 20 custom roles. Create a `Student-ReadNoFork` role: Base = Read, disable ability to create forks, create tags.
3. **Repo-level Fork Protection:** For your golden source `01-lecture-notes`, go to Repo > Settings > General > Features > Uncheck `Allow forking`. For organization-owned repositories, you can control forking - if you disable forking for a private repository, no one including org members will be able to fork it. You can also enforce at Org level: Settings > Member privileges > Repository forking policy.

> **Limitation to be honest about:** If you remove a person's access, any forks of that private repo are deleted, but local clones are retained. You cannot stop copy-paste, only mass redistribution.

## 3. PROTECTION METHODS - Comparison

| Method | What it does | Cost | When to use |
|---|---|---|---|
| **Private Repo + Team Read** | Only invited students can see/clone. No public link. | Free | Always - your baseline |
| **Disable Forking (Org + Repo)** | Prevents student creating a fork in their own account that they could make public later | Free | Always for `lecture-notes` |
| **Base Permission = No Permission** | Student removed from Team loses all access instantly | Free | Always |
| **Branch Protection on `main`** | Students cannot push/overwrite lecture notes. Requires PR from TAs only | Free | For `01-lecture-notes` |
| **CODEOWNERS file** | Only `@Instructors` can approve changes to notes | Free | Add to `01-lecture-notes` |
| **GitHub Classroom Private Assignments** | Student's assignment repo is private by default | Free with Education | For all labs |
| **Require Invite via Classroom Link** | Students don't need to understand Git - click link, get private copy | Free | Best for beginners |
| **SAML SSO / Domain Restrict / IP Allow List** | Only `@university.edu` users, or only on campus IP can clone | Enterprise Cloud | If your university has it |
| **Audit Log + Revoke at End** | See who cloned, when. Remove Cohort Team at end of term. | Free | End of semester |

Practical combo for free: **Private + No Base Permission + Disable Forking + Classroom Private + End-of-Term Removal** - covers 95% of accidental sharing.

## 4. CLASSROOM WORKFLOW - For Beginners

This is how you avoid teaching Git on day 1.

### One-time setup

1. classroom.github.com > Create Classroom > Link your `esp32-course-2026` Org
2. In your Org, create `lab01-blink-template`. Go to Repo > Settings > Check `Template repository`. Keep it private. Template repos should be private since Classroom can access them regardless.

### Per Assignment

1. Classroom > New Assignment > Title: `Lab01-GPIO`
2. Select template: `lab01-blink-template`
3. Visibility: **Private** - students only see their own.
4. Grant admin to TAs, not students.
5. Copy invite link, post to LMS.

Student flow: Clicks link > Authorizes > Gets `lab01-gpio-theirusername` private repo in your Org, pre-filled with starter code. They edit in Arduino IDE / VS Code + GitHub Desktop. You get auto-grading if you add Actions.

## 5. POLICY LAYER - You Must Add This

Technical controls fail without a written policy. Add these 3 files to `00-course-info`.

### A) LICENSE file - `LICENSE.md`

```text
Educational Use License - ESP32 Course 2026

Copyright (c) 2026 [Your Name / University]

This material is for enrolled students of [Course Code] only.
Permission is granted to clone, read, and use for coursework.

You may NOT:
- Make public, publish, re-upload to GitHub public, forums, or AI training sets
- Share outside the GitHub Organization
- Distribute PDFs/schematics to non-enrolled persons

All rights reserved. Violations will be handled under university academic policy.
License: CC BY-NC-ND 4.0 (No Derivatives, Non-Commercial, No Public Sharing)
```

### B) README Disclaimer - Top of every repo

```markdown
> ⚠️ PRIVATE COURSE MATERIAL - DO NOT SHARE
> This repository is private to `esp32-course-2026` Organization.
> Forking is disabled. Public mirroring is prohibited per course policy.
> If you received this outside GitHub, please delete and contact the instructor.
```

### C) Student Agreement Text - Put in Classroom first assignment

```text
By accepting this assignment, I agree:
1. I will keep all course materials (notes, code, schematics, PDFs) private.
2. I will not upload them to public GitHub repos, Chegg, Course Hero, or Discord servers outside class.
3. I understand access will be revoked after the semester and forks are tracked.
```

**Pro Tip for PDFs:** Add a GitHub Action to watermark PDFs with the student's GitHub username on download. Even a footer "Licensed to @student123" reduces sharing massively. Low-tech version: export PDFs with "Cohort 2026 - Do Not Distribute" footer.

## Instructor Checklist

- [ ] Org created, base permission = No permission, public repo creation = OFF, forking private repos = OFF
- [ ] Applied for GitHub Education benefits
- [ ] Teams created: Instructors (Owner), TAs (Maintainer), Cohort-2026-S2 (Read)
- [ ] `01-lecture-notes` is Private, Team Read-only, Forking OFF, Branch protection ON, CODEOWNERS = @Instructors
- [ ] Template repos marked as Template, Private
- [ ] Classroom linked, test assignment creates private student repo
- [ ] LICENSE + README disclaimer in all repos
- [ ] Student agreement posted to LMS
- [ ] End-of-semester: Classroom > Archive, Org > People > Remove Cohort Team members

This keeps your costs at $0, your workflow beginner-friendly (students never need to `git init`), and your materials protected by both access control and policy. You can't stop a determined student from copy-pasting, but you can prevent the common case where one student forks your lecture notes to public and 300 people star it.

## Starter LICENSE, README, CODEOWNERS, and branch protection ruleset files

Individual files/ sections:

- **LICENSE.md** - Educational non-distribution license. Put in root of every repo.
- **README.md** - Warning banner + quick start for beginners. Replace your main `01-lecture-notes` README with this.
- **CODEOWNERS** - Only Instructors/TAs can approve changes. Place in repo root AND in `.github/CODEOWNERS`
  - Copy: `.github/CODEOWNERS`
- **ruleset.json** + **BRANCH_PROTECTION_GUIDE.md** - Blocks direct push to `main`, requires PR + CODEOWNER review, blocks force-push and deletion.
- **STUDENT_AGREEMENT.md** - Post to LMS, have students sign before first Classroom link.
- **SETUP_CHECKLIST.md** - Your one-page Org hardening checklist.
- **watermark.yml** - Optional GitHub Action placeholder to stamp PDFs with `Licensed to @username`.

### Educational non-distribution license

Educational non-distribution license. Put in root of every repo.

```markdown
# Educational Use License - ESP32 Course Materials

Copyright (c) 2026 Peter Tan / [Your University Name]

This material is provided SOLELY for students officially enrolled in
[Course Code: e.g., EE4208 - IoT with ESP32] - Semester 2026.

## Permissions Granted
- Clone, pull, and read within the private GitHub Organization esp32-course-2026
- Use code examples (Arduino / ESP-IDF) for coursework, labs, and final project
- Modify template repositories assigned via GitHub Classroom for assignment submission

## Prohibited Actions
You may NOT, without prior written permission:
1. Make any part public - including public GitHub repos, Gists, forums, blogs, Chegg, Course Hero
2. Share, forward, or distribute PDFs, schematics, slides, or source code outside the Organization
3. Upload to AI training datasets or public file-sharing services
4. Fork to a personal public account (forking is disabled)

This license is based on CC BY-NC-ND 4.0 with additional non-distribution clause.

Enforcement: Violation will be treated as academic misconduct and may result in revocation of access.

Contact: [your-email@university.edu]
```

### Private Course Materials

Warning banner + quick start for beginners. Replace your main 01-lecture-notes README with this.

```markdown
# PRIVATE COURSE MATERIAL - DO NOT SHARE

> Repository: 01-lecture-notes
> Organization: esp32-course-2026
> Access: Private - Enrolled Students Only (Read-only)
> Forking: Disabled
> License: See LICENSE.md

This repository contains ESP32 lecture notes, schematics, and reference code for GPIO, Wi-Fi, BLE, FreeRTOS, Sensors, MQTT, IoT Projects.

## For Students - Quick Start
1. You have Read access only. You can clone/pull but cannot push directly.
2. Use git clone or GitHub Desktop. Do not try to fork.
3. For labs: Go to GitHub Classroom invite link posted on LMS.


git clone https://github.com/esp32-course-2026/01-lecture-notes.git


## Structure
- week01-gpio/ - GPIO, LED, Button, Interrupts
- week02-wifi/ - STA/AP mode, HTTP, MQTT
- week03-ble/ - BLE Server/Client
- week04-freertos/ - Tasks, Queues, Timers
- week05-sensors/ - I2C, SPI, DHT22, MPU6050
- schematics/ - KiCad / PDF
- pdf/ - Slides - Watermarked

## Policy Reminder
If you received this outside GitHub Organization, please delete and contact instructor.

Maintained by @Instructors - Issues welcome, PRs only from TAs.

```

### esp32-course-starter-pack/CODEOWNERS

Only Instructors/TAs can approve changes. Place in repo root AND in .github/CODEOWNERS

```markdown
# CODEOWNERS for esp32-course-2026/01-lecture-notes
# Only Instructors and TAs can approve changes

* @esp32-course-2026/instructors

/schematics/ @esp32-course-2026/instructors
/pdf/ @esp32-course-2026/instructors
/week* @esp32-course-2026/tas @esp32-course-2026/instructors
```

```json
{
  "name": "protect-main-lecture-notes",
  "target": "branch",
  "enforcement": "active",
  "conditions": {
    "ref_name": {
      "include": ["refs/heads/main"],
      "exclude": []
    }
  },
  "rules": [
    { "type": "deletion" },
    { "type": "non_fast_forward" },
    {
      "type": "pull_request",
      "parameters": {
        "required_approving_review_count": 1,
        "dismiss_stale_reviews_on_push": true,
        "require_code_owner_review": true,
        "require_last_push_approval": true
      }
    }
  ],
  "bypass_actors": [
    { "actor_id": 1, "actor_type": "Team", "bypass_mode": "always" }
  ]
}

```

### Branch Protection Guide

Blocks direct push to main, requires PR + CODEOWNER review, blocks force-push and deletion.

```markdown
# Branch Protection Setup for 01-lecture-notes

## UI Steps
1. Repo > Settings > Rules > Rulesets > New Ruleset > Branch
2. Name: protect-main-lecture-notes
3. Enforcement: Active
4. Target branches: main
5. Rules:
   - Restrict deletions: ON
   - Require pull request before merging: ON (Approvals:1, Require CODEOWNERS)
   - Require status checks: optional
   - Block force pushes: ON
6. Bypass: Allow Instructors team

Import ruleset.json via gh CLI: gh api -X POST /repos/ORG/REPO/rulesets --input ruleset.json

```
### STUDENT AGREEMENT

Post to LMS, have students sign before first Classroom link.

```markdown
# STUDENT AGREEMENT - ESP32 Course Materials

Course: [EE4208 - IoT with ESP32] - 2026
Organization: github.com/esp32-course-2026

By accepting assignment in GitHub Classroom or joining Org, I agree:

1. Confidentiality: All notes, PDFs, schematics, code in private Org are confidential.
2. No Public Sharing: I will NOT make public on public GitHub, Gist, blogs, Reddit, Discord outside class, Chegg, Course Hero.
3. No Redistribution: I will NOT forward to anyone not enrolled.
4. Technical Compliance: I will NOT bypass forking restrictions or remove watermarks.
5. Access Duration: Access revoked after semester. Delete local copies except my own code for portfolio.
6. Acknowledgement: Violations tracked via audit logs, treated as academic misconduct.

Name: __________________ GitHub: @______________ Date: _________ Signature: _________
```

### One-page Org hardening checklist

- Your one-page Org hardening checklist.
- watermark.yml (I couldn't attach this file. Want me to make it again?) - Optional GitHub Action placeholder to stamp PDFs with Licensed to @username.

```markdown
# ORGANIZATION SETUP CHECKLIST

- [ ] Create Org Free plan
- [ ] Settings > Member privileges: Base=No permission, Public repo creation OFF, Forking private OFF, 2FA ON
- [ ] Teams: Instructors (Owner), TAs (Maintainer), Cohort-2026-S2 (Member)
- [ ] Create repos as Private, disable forking
- [ ] Add CODEOWNERS, LICENSE.md, README.md
- [ ] Create Ruleset from ruleset.json
- [ ] Link GitHub Classroom to Org
- [ ] Test with dummy student account
```



### How to deploy in 5 minutes

#### 1. For `01-lecture-notes` repo

- Create as **Private**
- Settings > General > Uncheck "Allow forking"
- Upload `LICENSE.md`, `README.md`, `CODEOWNERS`
- Settings > Rules > Rulesets > Import `ruleset.json` -> Set Bypass to your `Instructors` Team

#### 2. For template repos like `lab01-blink-template`

- Create as Private > Settings > Check "Template repository"
- Same LICENSE + README but add lab instructions

#### 3. GitHub Classroom

Link Classroom to your Org. When you create assignment `Lab01`, choose template `lab01-blink-template` and set visibility to **Private** - Classroom auto-creates `lab01-@student` as private, no forking needed.

This gives you: Private-only access, no public forks, Read-only for students on notes, private per-student copies for labs, and a legal/policy layer that actually deters sharing more than any technical lock.

---

# Classroom 50 — Complete Guide

<img width="50%" height="auto" alt="image" src="https://github.com/user-attachments/assets/74c81a42-20d0-4788-9551-93471c63fc1a" />


## Classroom 50

Free and open source  
Developed by the Fifty Foundation · Launched July 1, 2026

[Official Classroom 50 website](https://classroom50.org/) · [GitHub source repository](https://github.com/foundation50/classroom50)

## 1. What is Classroom 50?

Classroom 50 is a free, open-source educational platform developed by the Fifty Foundation as an alternative to GitHub Classroom.

It allows instructors to distribute programming assignments, create private student repositories, run automated tests, and manage submissions using GitHub as the underlying repository and computing infrastructure.

It is particularly relevant to your ESP32 course because it retains the familiar GitHub workflow:

1. Instructor prepares a starter-code repository.
2. Classroom 50 distributes the assignment.
3. Each student receives an individual GitHub repository.
4. Students clone their repositories and work locally.
5. Students commit and push their changes.
6. Automated tests can run through GitHub Actions.
7. Instructors review the submissions.

The first version was released on July 1, 2026. GitHub's official transition announcement identifies Classroom 50 as an open-source alternative to the retired GitHub Classroom.  
GitHub+2

An important distinction: Classroom 50 is not a replacement for your GitHub Organization. It is an application that works with your existing GitHub Organization and repositories.

Your existing organization remains the central place for managing students, teaching materials, teams, and repository permissions.

## 2. What can Classroom 50 do?


### 1. Assignment distribution

<img width="70%" height="Auto" alt="image" src="https://github.com/user-attachments/assets/c81afa7e-6b27-4b67-8d58-0a62c4921a5d" />


Create programming assignments using instructor-prepared starter repositories. Students receive their own assignment repositories instead of modifying the original template.


### 2. Private student repositories

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/c1072107-7379-40fe-adf6-e02a5cee6e0e" />


Students work independently in their own GitHub repositories. You can configure repository ownership and access through the connected GitHub organization.



### 3. Automated grading

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/335f4571-aaf8-44dc-a420-90a5e86f365a" />


Define correctness tests and use GitHub Actions to execute them when students push code. This provides automated feedback on programming assignments.



### 4. Submission management

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/0f4a52ee-1652-44e4-91a4-8a8da9d027cd" />

Manage assignment submissions and review student work through the associated GitHub repositories.



### 5. Git-based workflow

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/d4b224d1-1e9d-4e0b-b331-c35fc7d0ae52" />

Students can use Git Bash, Windows PowerShell, or macOS Terminal. They can also use GitHub through their browser for repository management.  

- [Introducing Classroom 50](https://github.com/foundation50/classroom50/discussions/46)
- [Transition Plan for GitHub Classroom](https://github.com/github-education-resources/teacher-toolbox/blob/main/admin/classroom-transition.md)
- [classroom50/wiki/troubleshooting](https://github.com/foundation50/classroom50/blob/main/wiki/Troubleshooting.md)


## 3. Important requirements before installation

This is the most important point to check before proceeding.

### GitHub Organization plan

**Required:** Classroom 50 requires your GitHub Organization to be on GitHub Team or Enterprise. A normal GitHub Free organization is not sufficient.

However, verified educators can obtain GitHub Team for their organization free through GitHub Education.

**Recommended:** GitHub Team through verified educator benefits — S$0.

Student accounts can remain on GitHub Free.

[Apply for GitHub Education](https://education.github.com/discount_requests/application)  


### 3.1 Requirements checklist

| Requirement | Needed? |
|---|---|
| Personal GitHub account | Yes |
| GitHub Organization | Yes |
| Organization on GitHub Team or Enterprise | Yes |
| Verified GitHub educator benefit | Required for free Team upgrade |
| GitHub CLI | No, when using the web application |
| Arduino IDE | Not needed for Classroom 50 setup |
| ESP-IDF | Not needed for Classroom 50 setup |
| Student GitHub accounts | Yes |
| GitHub Actions | Used for automated grading |
| GitHub Pages | Used by Classroom 50 for publishing its classroom data |

An important distinction: GitHub Education approval on your personal account does not automatically upgrade your organization. You must apply the benefit to the specific organization that hosts your ESP32 course.  


## Part B — Integrate Classroom 50 with Your Existing Organization

## 4. Understand the integration architecture

You already have a GitHub Organization. Classroom 50 will use that organization rather than creating a separate one.

Here is the proposed architecture:

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/12a200ba-ef3f-4960-ad05-e068733c91d1" />


```text
Your GitHub Organization
ESP32-Learning-Lab
Central identity, repository and permission management

Authorized GitHub connection

Classroom 50 Web Application
Teacher dashboard · Rosters · Assignments · Grading

One-time organization setup

classroom50
Private configuration repository

GitHub Actions
Automated grading and score collection

Course templates
Instructor-controlled starter code

Student repositories
Individual private assignments
```

A particularly useful design feature is that Classroom 50 does not maintain a separate classroom database. Its classroom configuration, roster, assignments and scores are stored in your GitHub organization. The web application operates in your browser and uses GitHub as its backend.  


## 5. Step 1 — Upgrade your existing Organization to GitHub Team

First, check your current plan.

1. Sign in to GitHub.
2. Open your ESP32 Organization.
3. Select **Settings**.
4. Open **Billing & licensing** or the current billing overview.
5. Check the organization plan.

If it shows GitHub Free:

1. Complete the GitHub Education teacher verification process.
2. Wait for approval.
3. Open your GitHub Education benefits page.
4. Find the GitHub Team benefit.
5. Apply it to your existing ESP32 Organization.
6. Return to your organization billing page.
7. Confirm that the organization now shows GitHub Team.

Official guides:

- [GitHub Education teacher application](https://docs.github.com/en/education/about-github-education/github-education-for-teachers/apply-to-github-education-as-a-teacher)
- [Educator eligibility and benefits](https://docs.github.com/en/education/about-github-education/github-education-for-teachers/apply-to-github-education-as-a-teacher)

Do not create a second organization just to use Classroom 50. Upgrade the one you have already configured.

## 6. Step 2 — Authorize Classroom 50 to access your Organization

**Browser only — no software installation**

1. Open [Classroom 50](https://classroom50.org/).
2. Click **Sign in with GitHub**.
3. Sign in using your instructor GitHub account.
4. GitHub will display an authorization page.
5. Review the requested permissions.
6. Authorize Classroom 50.
7. If GitHub asks you to grant organization access:
   - Find your ESP32 Organization.
   - Click **Grant**.
8. If it shows **Request**, rather than **Grant**, your organization may require approval of third-party applications.
9. As organization owner, approve Classroom 50 under:

```text
Organization → Settings → Third-party access → OAuth app policy
```

The exact menu labels can vary.

10. Return to Classroom 50.
11. Refresh the organization list.
12. Confirm that your ESP32 Organization appears.

**Important:** Classroom 50 only displays organizations for which you have granted it access. Being an organization owner or having GitHub Education benefits does not automatically grant this application access.  
GitHub+1

## 7. Step 3 — Run the one-time organization setup

This is the most important integration step.

After authorizing your organization:

1. Classroom 50 should display your organization.
2. Select **Set up new organization**.
3. Select your ESP32 Organization.
4. Click **Set up**.
5. Click **Run setup**.

Classroom 50 will configure several things in GitHub automatically.

| Configuration | Purpose |
|---|---|
| Organization member privileges | Establish settings needed for classroom operation |
| GitHub Actions | Enable automated grading workflows |
| Actions spending cap | Set a zero-dollar spending cap where possible |
| Organization rulesets | Protect student repositories |
| `classroom50` repository | Store classroom configuration and data |
| Workflow files | Support automated grading and score collection |
| GitHub Pages | Publish classroom information required by the application |

The `classroom50` repository is created as a private repository in your organization.

When setup finishes, review the result of every step. If an operation cannot be applied automatically, Classroom 50 explains what needs to be changed manually.

**Security consideration for your existing setup:** Classroom 50 modifies some organization-level settings. Review these carefully, especially member privileges, team creation, Actions and repository rulesets. Do not assume your previous organization settings will remain unchanged.  
GitHub+1

## 8. Step 4 — Configure the Classroom 50 service token

**Important security step**

Classroom 50 needs a service token to perform background tasks when you are not signed in.

For example:

- Collecting student scores.
- Reading student assignment repositories.
- Regrading submissions.
- Publishing grading results.

This is separate from the GitHub authorization you completed in Step 2.

### 8.1 Generate the token

After organization setup:

1. Click **Next: service token**.
2. Select **Set a token**.
3. Choose the token expiry. The documented default is 120 days.
4. Click **Generate new access token**.
5. GitHub opens its fine-grained personal access token creation page.
6. Classroom 50 pre-populates the required token name and permissions.
7. Select your ESP32 Organization as the resource owner.
8. Set **Repository access** to **All repositories**.
9. Review the permissions generated by Classroom 50.
10. Click **Generate token**.

Do not manually reduce the permissions unless you understand which Classroom 50 operations depend on them.

### 8.2 Save the token

1. Copy the newly generated token.
2. Return to Classroom 50.
3. Paste it into the service token field.
4. Click **Save token**.

```text
Token saved
when validation succeeds.
```

The token is saved as a GitHub Actions secret named:

```text
CLASSROOM50_SERVICE_TOKEN
```

inside your private `classroom50` repository.

### 8.3 Test the token

1. Click **Test token** in the organization settings.
2. Classroom 50 will run a read-only check to confirm that the token has the necessary permissions.
3. If the check fails, review the missing permission information and generate a replacement token.

### 8.4 Token management

I recommend adding the token expiry date to your instructor administration calendar.

1. Check the token status periodically.
2. Generate a replacement before expiry.
3. Save the new token through Classroom 50's **Set a new token** function.

- Never put this token in Arduino sketches, ESP-IDF source files, or student repositories.
- Never share it with students or paste it into a chat.

The token is important because it allows background workflows to access repositories across your organization.  
GitHub+1

## 9. Step 5 — Create your ESP32 classroom

After the organization setup and service token configuration:

1. Click **Done**.
2. Open your ESP32 Organization.
3. Click **Create classroom**.
4. Use these suggested settings:

| Field | Suggested value |
|---|---|
| Classroom name | ESP32 Embedded Systems |
| Classroom slug | `esp32-2026` |
| Term | 2026 |
| Unlisted link | Initially disabled |

You can create different classrooms for different cohorts or class groups within the same organization.

For example:

```text
ESP32-Learning-Lab
│
├── ESP32 Embedded Systems 2026
│   └── esp32-2026
│
├── ESP32 Embedded Systems 2027
│   └── esp32-2027
│
└── ESP32 Advanced IoT
    └── esp32-advanced
```

**Security note:** Classroom 50 supports unlisted classroom links, but these are not private access controls. Anyone with the published link may be able to read the associated published data. Do not use an unlisted link as a substitute for private repositories or membership controls.  
GitHub

## 10. Step 6 — Add your Teaching Assistants

Inside your classroom:

1. Open **Settings**.
2. Find **Staff and roles**.
3. Enter the TA's GitHub username.
4. Select the appropriate role.
5. Click **Add**.

Classroom 50 supports four roles:

| Role | Suggested person | Responsibility |
|---|---|---|
| Teacher | You | Full classroom administration |
| Head TA | Senior teaching assistant | Manage teaching operations |
| TA | Teaching assistant | Assist with reviewing and grading |
| Student | Course participant | Accept and complete assignments |

These are Classroom 50 classroom roles, not identical to GitHub's repository permission roles.

For your course, I recommend:

- You: Teacher.
- Senior TA: Head TA.
- Other TAs: TA.
- Students: Student.

You should retain organization ownership rather than relying solely on the Classroom 50 Teacher role for administrative control.  
GitHub

## Part C — Prepare Your ESP32 Assignments

## 11. Step 7 — Prepare the assignment template repositories

This is where your existing ESP32 teaching materials become useful.

I recommend keeping your main teaching materials repository separate from your assignment templates.

For example:

```text
ESP32-Learning-Lab
│
├── esp32-course-materials
│   ├── 01-GPIO
│   ├── 02-WiFi-BLE
│   ├── 03-FreeRTOS
│   ├── 04-Sensors-IoT
│   ├── 05-Memory-PSRAM
│   └── 06-Secure-OTA
│
├── esp32-code-examples
│   ├── Arduino
│   └── ESP-IDF
│
├── esp32-instructor-only
│   ├── solutions
│   └── grading-rubrics
│
├── esp32-assignment-gpio
├── esp32-assignment-wifi
├── esp32-assignment-freertos
└── classroom50
```

### 11.1 Create the first assignment template

Let's start with GPIO.

Create a private repository:

```text
esp32-assignment-gpio
```

Suggested folder structure:

```text
esp32-assignment-gpio/
│
├── README.md
├── LICENSE
│
├── Arduino/
│   ├── GPIO_Exercise.ino
│   └── README.md
│
├── ESP-IDF/
│   ├── CMakeLists.txt
│   └── main/
│       ├── CMakeLists.txt
│       └── main.c
│
├── schematics/
│   └── gpio_connection.png
│
└── .github/
    └── pull_request_template.md
```

The `README.md` should explain:

- Assignment objectives.
- Required ESP32 board.
- Hardware connections.
- GPIO pins to use.
- Expected program behaviour.
- Submission requirements.
- Testing criteria.

### 11.2 Enable GitHub template repository

In the GitHub browser:

1. Open `esp32-assignment-gpio`.
2. Select **Settings**.
3. Under **General**, find **Template repository**.
4. Enable **Template repository**.

This allows Classroom 50 to use the repository as the starting point for individual student repositories.

### 11.3 Keep solutions separate

Do not include the complete instructor solution in the template.

For example:

```text
esp32-instructor-only/
│
├── GPIO/
│   ├── solution.ino
│   ├── solution.md
│   └── grading-rubric.md
│
├── WiFi/
│   └── solution/
│
└── FreeRTOS/
    └── solution/
```

Students should receive only the starter code and assignment instructions.

## 12. Step 8 — Create your first assignment in Classroom 50

Return to your Classroom 50 dashboard.

1. Open **ESP32 Embedded Systems 2026**.
2. Select **Assignments**.
3. Click **Assignment**.

The assignment creation screen has four main sections.

### Section A — Details

Use these settings:

| Field | Example |
|---|---|
| Assignment name | ESP32 GPIO Exercise 01 |
| Assignment slug | `gpio-01` |
| Description | Digital GPIO input and output programming |
| Assignment type | Individual |

For a first exercise, I recommend individual assignments.

Group assignments are also supported, but individual assignments are easier to manage for beginners.

### Section B — Repository setup

Configure:

| Field | Recommended setting |
|---|---|
| Start with a template | Template repository |
| Template | `ESP32-Learning-Lab/esp32-assignment-gpio` |
| Repository visibility | Private |
| Student repository access | Write |
| Feedback pull request | Enabled |
| Include all branches | Disabled unless needed |

The student repository should be private.

Students need `Write` access to their own repository so that they can push completed code.

The feedback pull request option provides a convenient place for instructor feedback.

### Section C — Submission and grading

There are three main grading modes:

| Mode | Description | Recommended use |
|---|---|---|
| Not graded | Collect submissions without assigning scores | Practice exercises |
| Manual | Instructor enters scores | Hardware demonstrations and practical assessments |
| Autograded | Tests execute automatically | Programming exercises with testable outputs |

For your first GPIO assignment, I suggest **Manual grading** until you have tested the automated grading workflow.

### Section D — Schedule and access

Set:

- Release date.
- Due date.
- Assignment lock, if required.

The due date identifies late submissions but does not automatically prevent students from pushing changes. Use the close-submission function when you actually want to stop submissions.

Finally, click **Create assignment**.

The settings and assignment options above follow the current Classroom 50 Web Teacher Guide.  
GitHub+1

## 13. Step 9 — Add students to your classroom

This is where Classroom 50 connects your actual students to their GitHub accounts.

There are two ways to add students.

### Option A: Add students individually

1. Open your classroom.
2. Select **Roster**.
3. Click the arrow beside **Upload roster**.
4. Select **Add member**.
5. Select the role **Student**.
6. Enter the student's GitHub username.
7. Optionally enter their name and email.
8. Click **Add member**.

Classroom 50 sends an invitation to join your GitHub Organization.

The student must accept the invitation before accepting assignments.

### Option B: Upload the student roster (recommended)

For a class of 20–30 students, this is easier.

Prepare a CSV file:

```csv
username,first_name,last_name,email,section
studentA,John,Tan,john@example.com,A
studentB,Mary,Lim,mary@example.com,A
studentC,David,Lee,david@example.com,A
```

Then:

1. Open **Roster**.
2. Click **Upload roster**.
3. Select your CSV file.
4. Review the imported student records.
5. Confirm the import.

Classroom 50 sends the organization invitations.

You can also import a simple list of GitHub usernames or email addresses.

**Important:** Students must have their own GitHub accounts. If you invite students by email, Classroom 50 can match them to their GitHub accounts when they accept.

The roster interface shows which students have joined and which invitations are still pending.  
GitHub

## 14. Step 10 — Distribute the assignment

After creating the assignment and adding students:

1. Open the assignment.
2. Find the student invitation or assignment link.
3. Click **Share** where available.
4. Copy the assignment invitation link.
5. Send it to your students through your normal communication channel.

### Student workflow

1. **Receive assignment link**  
   Student opens the Classroom 50 invitation in a browser.
2. **Sign in with GitHub**  
   Student uses their own GitHub account.
3. **Accept assignment**  
   Classroom 50 creates an individual private repository from your template.
4. **Clone repository**  
   Student downloads the repository to their Windows or Mac computer.
5. **Complete ESP32 exercise**  
   Student edits Arduino or ESP-IDF code and tests it on their ESP32 board.
6. **Commit and push**  
   Student uploads their work to their private GitHub repository.
7. **Instructor reviews submission**  
   Classroom 50 collects the submission and displays grading information.

The important advantage is that students do not need to create their own assignment repositories manually.

### Example resulting repositories

For an assignment called `gpio-01`, the repositories will follow a classroom and assignment naming convention:

```text
ESP32-Learning-Lab
│
├── esp32-2026-gpio-01-studentA
├── esp32-2026-gpio-01-studentB
├── esp32-2026-gpio-01-studentC
└── esp32-2026-gpio-01-studentD
```

Each student works in their own private repository.

Your original `esp32-assignment-gpio` template remains separate from their submissions.  
GitHub+1

## Part D — Automated Grading for ESP32

## 15. What can Classroom 50 automatically grade?

Classroom 50 supports three built-in test types:

| Test type | How it works | ESP32 application |
|---|---|---|
| Input/Output | Compares program output against expected output | Testing parsing functions or command-line utilities |
| Run command | Executes a command and checks its exit code | Compile checks and script execution |
| Python pytest | Executes Python unit tests | Testing Python tools and supporting applications |

You can also provide custom grading scripts and configure a Docker environment.

Tests can be assigned individual point values, and Classroom 50 can produce a score based on the test results.  


## 16. Important: ESP32 hardware cannot be tested automatically by an ordinary GitHub-hosted runner

This distinction is particularly important for your course.

GitHub Actions normally executes code on a cloud computer, not on the student's physical ESP32-S3 board.

For example, GitHub Actions cannot directly verify:

- Whether GPIO 4 physically drives an LED.
- Whether an MFRC522 RFID reader is wired correctly.
- Whether an ILI9488 display actually shows the correct image.
- Whether an ESP32-S3 successfully communicates with a sensor over I²C.
- Whether an ESP-NOW message is received by another physical ESP32.

You can, however, automatically check code-level requirements.

### My recommended grading strategy

```text
ESP32 assignment grading model

Automated
40% — Source-code and compilation checks
- Required source files exist.
- Required functions are implemented.
- Code compiles using a configured Arduino CLI or ESP-IDF environment.
- Static checks pass.
- Unit tests pass.

Manual
40% — Physical hardware demonstration
- ESP32 GPIO behaviour.
- Sensor readings.
- Communication reliability.
- Display output.
- Correct circuit connections.

Manual
20% — Technical explanation
- Explain program structure.
- Explain GPIO configuration.
- Explain FreeRTOS task behaviour where relevant.
- Explain troubleshooting and design decisions.
```

These percentages are an example grading scheme, not a Classroom 50 limitation.

### Example: GPIO assignment

Suppose students must write an Arduino sketch that:

- Configures GPIO 4 as an output.
- Configures GPIO 5 as an input.
- Reads the input state.
- Controls an LED.
- Prints the input state to Serial.

Possible automated tests:

| Test | Points |
|---|---|
| Required Arduino source file exists | 10 |
| GPIO 4 configuration detected | 10 |
| GPIO 5 configuration detected | 10 |
| Required function implemented | 20 |
| Arduino compilation succeeds | 30 |
| Unit tests for logic pass | 20 |
| **Total** | **100** |

The physical LED and input operation should still be demonstrated by the student.

**My advice:** Start with manual grading. Once you have a stable assignment template, gradually introduce automated compilation and unit testing.

## 17. Teacher feedback and submission tracking

Classroom 50 supports:

- Submission tracking.
- Scores and grading results.
- Inline feedback through feedback pull requests.
- Regrading.
- Collecting scores for an entire classroom.
- Reviewing student repository changes and commit histories.

### How to review submissions

1. Open the classroom.
2. Select the assignment.
3. Open its submissions page.
4. Review the list of students.
5. Check submission status.
6. Open the student's repository when necessary.
7. Review the submitted code.
8. Enter manual scores or review automated results.

For automated assignments, you can use **Collect now** or **Collect all** to collect scores. The background collection workflows depend on the service token configured earlier.  
GitHub+1

## Part E — Security, Privacy and Cost Considerations

## 18. Does Classroom 50 protect your ESP32 course materials?

Yes, it provides useful access controls, but it is important to understand what it protects and what it does not.

| Security requirement | Classroom 50 / GitHub capability |
|---|---|
| Keep assignment repositories private | Supported |
| Prevent students modifying instructor templates | Use private templates and read-only access |
| Give each student an individual repository | Supported |
| Isolate student submissions | Supported through individual repositories and permissions |
| Prevent unauthorized public forking | Configure organization forking policy |
| Prevent students downloading source code | Not possible while allowing cloning |
| Prevent copying downloaded materials | Not possible |
| Revoke organization access | Supported through GitHub membership management |
| Keep grading configuration private | Private `classroom50` repository |
| Protect background workflows | Fine-grained service token stored as a GitHub Actions secret |

### Three additional precautions I recommend

**1. Keep student assignment repositories private.**

Do not enable public repository visibility for ordinary coursework.

**2. Keep your original teaching materials separate.**

Do not give students `Write` or `Admin` access to your original course repositories.

**3. Be careful with autograder test files.**

There is an important detail in Classroom 50's current documentation: uploaded teacher-only test files are protected from student modification, but they are also published through the organization's public GitHub Pages site and can be read by students.

Therefore, never put complete instructor solutions, answer keys, signing keys, or confidential grading information in these test files. Use them for test scripts and test fixtures only.  
GitHub+1

## 19. How does Classroom 50 affect your existing Teams?

You previously created:

- `Instructors`
- `TAs`
- `Cohort-2026`

You can retain these teams.

However, Classroom 50 maintains its own classroom-specific teams and staff/roster structure.

I recommend the following separation:

| Existing GitHub team | Purpose |
|---|---|
| `Instructors` | Organization and course administration |
| `TAs` | General course support |
| `Cohort-2026` | Read-only access to common course materials |

Classroom 50 then manages student enrollment and assignment access through its own classroom mechanisms.

Do not manually give the entire `Cohort-2026` team `Write` access to assignment repositories. Let Classroom 50 manage individual student permissions.

Also, review its organization setup changes before allowing students to join. In particular, student-created teams are needed if you intend to use student-formed group assignments.  
GitHub+1

## 20. Cost considerations

**Target: S$0**

| Component | Cost approach |
|---|---|
| Classroom 50 | Free and open source |
| GitHub Team | Free for verified educators through GitHub Education |
| Student GitHub accounts | Free |
| Private repositories | Included in the applicable GitHub Team plan |
| GitHub Actions | Included usage quotas, subject to current plan limits |
| GitHub Pages | Available under the supported Team configuration |
| Arduino IDE | Free |
| ESP-IDF | Free |

Classroom 50's organization setup attempts to configure a zero-dollar Actions spending cap where possible. This is useful protection against unexpected charges, but you should still review the organization's billing and Actions usage periodically.  
GitHub+1

## Part F — Semester-End Administration

## 21. What happens when the semester ends?

Classroom 50 does not automatically delete all student repositories when a student is removed from the roster.

You need to manage the lifecycle yourself.

### Recommended procedure

1. Collect all assignment scores.
2. Export or record required grading information.
3. Close assignment submissions.
4. Remove students from the classroom or organization according to your access policy.
5. Review direct repository permissions.
6. Check student assignment repositories.
7. Retain or archive submissions according to your institutional requirements.
8. Rotate or review service tokens as appropriate.

Removing a student from the roster and deleting their repository are separate operations.

This is another reason to maintain your own instructor backup.  
GitHub+1

## Part G — Your First Implementation

## 22. Recommended rollout sequence

I suggest that you do not migrate all your ESP32 teaching materials immediately.

Start with one small pilot assignment.

```text
ESP32 Classroom 50 implementation checklist
0 of 12 completed
0%

[] Verify GitHub Education educator status
Apply for educator verification.

[] Upgrade existing organization
Apply free GitHub Team to ESP32-Learning-Lab.

[] Authorize Classroom 50
Sign in and grant organization access.

[] Run organization setup
Create classroom50 repository and configure Actions and Pages.

[] Configure service token
Generate and test the fine-grained token.

[] Create classroom
ESP32 Embedded Systems 2026.

[] Add TAs
Assign appropriate Classroom 50 roles.

[] Prepare GPIO template
Create esp32-assignment-gpio.

[] Create first assignment
GPIO Exercise 01, individual, private.

[] Add 2 test students
Verify student invitation and assignment acceptance.

[] Test student workflow
Clone, edit, commit and push.

[] Review submissions
Verify private access and instructor feedback.
```

## 23. Official Classroom 50 resources

These are the most useful references for your implementation.

### 1. Classroom 50 Web Teacher Guide

**Most important**  
Detailed browser-based instructions covering organization setup, service tokens, classrooms, assignments, students and grading.

[Open Web Teacher Guide](https://github.com/foundation50/classroom50/wiki/Web-Teacher-Guide)

### 2. Quickstart

A practical walkthrough to create your first classroom and graded assignment.

[Open Quickstart](https://github.com/foundation50/classroom50/wiki/Quickstart)

### 3. Assignment Templates

How to prepare and configure your starter-code repositories.

[Open Assignment Templates](https://github.com/foundation50/classroom50/wiki/Assignment-Templates)

### 4. Autograding Basics

Learn how to configure automated tests and grading.

[Open Autograding Basics](https://github.com/foundation50/classroom50/wiki/Autograding-Basics)

### 5. Course Lifecycle and End of Term

Manage students, submissions and the end-of-semester process.

[Open lifecycle guide](https://github.com/foundation50/classroom50/wiki/Course-Lifecycle-and-End-of-Term)

### 6. Troubleshooting

Useful for organization authorization, missing organizations, token errors and setup problems.

[Open Troubleshooting](https://github.com/foundation50/classroom50/wiki/Troubleshooting)

## My final recommendation for your ESP32 Learning Lab

I recommend adopting Classroom 50, subject to getting the free GitHub Team benefit.

It is a good fit for your teaching model because it allows you to keep your existing instructor materials in private repositories while distributing individual assignment copies to students.

I would implement it in this order:

1. Get your existing organization upgraded to GitHub Team.
2. Complete Classroom 50 integration using the browser.
3. Create one GPIO assignment.
4. Test with two student accounts.
5. Introduce automated compilation checks after the basic workflow works.
6. Gradually migrate your Wi-Fi, BLE, FreeRTOS, sensor and secure OTA assignments.

One particularly important design decision: Keep `esp32-course-materials` as your permanent instructor-controlled source repository. Classroom 50 should distribute assignment-specific copies, not replace your main teaching repository.

That way, you can update the master ESP32 course materials without unintentionally changing existing student submissions.

---

# GitHub Gists

GitHub Gists is a GitHub feature that allows you to share small pieces of code, configuration files, notes or text without creating a complete GitHub repository.

Think of it as a lightweight code-sharing service or an online code snippet library.

[Open GitHub Gist](https://gist.github.com/)

## 1. What can you do with Gists?

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/a5f8c497-1e75-4d8a-8040-1e8fb4ddd342" />


[docs.github.com](https://docs.github.com/en/get-started/writing-on-github/editing-and-sharing-content-with-gists/creating-gists)

Typical applications:

- Share a short Arduino ESP32 code example.
- Publish a reusable C++ function.
- Share configuration files.
- Keep useful command-line instructions.
- Store code snippets for teaching.
- Share debugging logs with students.

For example, you might create a Gist containing:

```cpp
// ESP32-S3: Check PSRAM availability

void setup() {
  Serial.begin(115200);

  if (psramFound()) {
    Serial.printf("PSRAM: %u bytes\n",
                  ESP.getPsramSize());
  } else {
    Serial.println("PSRAM not found");
  }
}

void loop() {
}
```

You can share the Gist URL with students without asking them to clone a repository.

## 2. Public Gist vs Secret Gist

| Feature | Public Gist | Secret Gist |
|---|---|---|
| Discoverable through GitHub search | Yes | No |
| Anyone with URL can view | Yes | Yes |
| Can be shared | Yes | Yes |
| Version history | Yes | Yes |
| Can be cloned | Yes | Yes |

**Important**

Secret Gists are not private repositories. Anyone who obtains the URL can access them.

Never store passwords, API tokens, Wi-Fi credentials or GitLab deployment tokens in a Gist.

## 3. Gist vs Repository

| Feature | Gist | GitHub Repository |
|---|---|---|
| Main purpose | Share code snippets | Manage complete projects |
| Multiple files | Yes | Yes |
| Version control | Yes | Yes |
| Issues and pull requests | No standard repository workflow | Yes |
| GitHub Actions | No normal repository workflow | Yes |
| GitHub Classroom | Not intended for assignments | Yes |
| Project documentation | Basic | Full project support |

## 4. How I would use Gists in your ESP32 Learning Lab

I would use Gists as a supplementary teaching resource rather than your main course repository.

| Teaching material | Recommended location |
|---|---|
| Complete ESP32 lessons | GitHub repository |
| Student assignments | GitHub Classroom |
| Reusable ESP32 code snippets | GitHub Gists |
| ESP32 libraries | GitHub repository |
| Firmware binaries | GitLab Package Registry |
| Automated compilation | GitHub Actions |

For example, you could create a collection of Gists:

- ESP32 GPIO initialization
- ADC reading and voltage conversion
- PWM motor control
- I²C device scanning
- SPI communication
- PSRAM allocation
- FreeRTOS task creation
- ESP-NOW communication
- Wi-Fi connection troubleshooting

## 5. Creating your first Gist

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/b52a920b-e3cd-4209-a7c4-18387c0c41bc" />




1. Visit [gist.github.com](https://gist.github.com/).
2. Sign in with your GitHub account.
3. Enter a filename, for example `ESP32_PSRAM_Test.ino`.
4. Paste your code.
5. Add a description.
6. Choose **Create public gist** or **Create secret gist**.
7. You will receive a URL that you can share with students.

**My recommendation:** Gists are particularly useful for your ESP32 teaching activities when you want to distribute small, standalone code examples without creating a separate repository for every example. Keep your official course materials and student assignments in your existing GitHub organization.

---

# GitHub Actions – Introduction, Features and Step-by-Step Setup

**Recommended for your ESP32 Learning Lab**

GitHub Actions is one of the most useful features of GitHub because it allows you to automate software development, compilation, testing, validation and deployment directly from your GitHub repositories.

For your ESP32-S3 projects and GitHub Classroom teaching environment, GitHub Actions can become an automated teaching assistant:

1. Students submit their Arduino code.
2. GitHub automatically detects the submission.
3. An automated environment compiles the ESP32-S3 firmware.
4. Tests and validation checks are performed.
5. Results are reported back to the student and instructor.

You do not need to manually download every student's code and compile it in Arduino IDE.

Let's go through the concepts first, followed by a practical setup.

## 1. What is GitHub Actions?

GitHub Actions is a Continuous Integration and Continuous Delivery/Deployment (CI/CD) platform built into GitHub.

In traditional software development, developers often perform these activities manually:

- Write source code.
- Compile the program.
- Run tests.
- Check for errors.
- Package the application.
- Release the software.

GitHub Actions automates these activities using predefined workflows.

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/3ede542a-ecfb-493b-84a4-72299f4e33d7" />




For example, whenever you upload a new ESP32 program to GitHub, an Actions workflow can automatically:

- Download the source code.
- Install the ESP32 Arduino development environment.
- Compile the program for ESP32-S3.
- Check whether compilation succeeds.
- Save the generated `.bin` firmware.
- Report the result on GitHub.

This is called Continuous Integration (CI).

When the workflow also publishes the firmware or deploys a website, that is part of Continuous Delivery/Deployment (CD).

## 2. Understanding the main components

Before setting up Actions, understand five important terms.

### 1. Workflow

A workflow is an automated process defined in a YAML file.

Example: `esp32-build.yml`

It specifies what to do, when to execute and which operations to perform.

### 2. Event / Trigger

An event determines when a workflow starts.

Examples:

- Student pushes new code.
- A pull request is submitted.
- An instructor manually starts a workflow.
- A scheduled time is reached.

### 3. Job

A job is a collection of steps that execute on a runner.

Example:

- Install Arduino CLI.
- Install ESP32 board support.
- Compile firmware.

### 4. Runner

A runner is the computer that executes the workflow.

GitHub provides hosted runners such as Ubuntu Linux, Windows and macOS.

You can also configure your own self-hosted runner.

### 5. Step

A step is an individual command or action inside a job.

Example:

```yaml
- name: Compile ESP32 firmware
  run: arduino-cli compile ...
```

## 3. GitHub Actions architecture

The following diagram illustrates how the components work together.

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/a8c691ab-eb04-4c93-bb46-f10915d82290" />


The execution sequence is:

1. An event occurs in a GitHub repository.
2. GitHub checks the workflow configuration.
3. A runner is allocated.
4. The runner executes the jobs and steps.
5. The workflow generates logs, reports and optional output files.
6. GitHub displays the execution result.

## 4. Important features of GitHub Actions

Here are the capabilities you can use in your projects.

| Feature | What it does | ESP32 application |
|---|---|---|
| Continuous Integration | Automatically builds and tests code | Compile every ESP32 submission |
| Continuous Deployment | Automatically publishes or deploys software | Deploy course website |
| Event triggers | Starts workflows when events occur | Trigger on student code submission |
| Scheduled workflows | Executes at specified times | Run nightly project checks |
| Manual execution | Allows users to start workflows | Instructor initiates a build |
| Matrix builds | Runs multiple configurations in parallel | Compile for ESP32-S3, ESP32-C3 and ESP32 |
| Secrets management | Stores encrypted credentials | Store deployment credentials |
| Artifacts | Saves generated files | Store compiled `.bin` firmware |
| Caching | Reuses downloaded dependencies | Reduce repeated ESP32 core downloads |
| Workflow reports | Displays logs and pass/fail status | Show students compilation errors |
| Reusable workflows | Shares automation across repositories | Apply one build procedure to all classes |

Some particularly useful features deserve more explanation.

### 4.1 Event triggers

You can decide exactly when automation should run.

| Trigger | Meaning | Example |
|---|---|---|
| `push` | New code uploaded | Student submits code |
| `pull_request` | Pull request opened or updated | Instructor reviews assignment |
| `workflow_dispatch` | Manual execution | Instructor starts compilation |
| `schedule` | Scheduled execution | Daily course checks |
| `release` | New release published | Publish firmware |

You can also restrict triggers to particular branches or changed files.

### 4.2 Matrix builds

This is a powerful feature for embedded development.

Suppose you want to compile one application for three different ESP32 targets.

Instead of creating three separate workflows, you can define a matrix:

```yaml
strategy:
  matrix:
    board:
      - esp32:esp32:esp32s3
      - esp32:esp32:esp32c3
      - esp32:esp32:esp32
```

GitHub can run these builds in parallel.

This helps identify board-specific compilation problems.

### 4.3 Secrets management

You can store confidential information in GitHub without putting it directly into your source code.

For example:

- GitLab deployment token
- Cloudflare API token
- Website deployment credentials
- Private package registry credentials

Your workflow accesses these values through GitHub's secrets mechanism.

Example:

```yaml
env:
  GITLAB_TOKEN: ${{ secrets.GITLAB_TOKEN }}
```

The actual token is configured separately under repository or organization settings.

**Security recommendation:** Never expose deployment secrets to workflows that execute untrusted student contributions. In particular, do not use `pull_request_target` to check out and execute untrusted pull-request code.

### 4.4 Artifacts

Artifacts are files produced by a workflow that you can download later.

For ESP32 projects, these could include:

- Compiled firmware `.bin`
- Compilation reports
- Test results
- Log files
- Generated documentation

For example, a student can submit an Arduino sketch and download the compiled firmware from the corresponding Actions run.

### 4.5 Workflow templates and Marketplace

You do not have to write everything yourself.

GitHub provides a Marketplace containing reusable actions for:

- Installing development tools
- Compiling applications
- Running automated tests
- Publishing packages
- Deploying websites
- Generating reports

You can find them here: [GitHub Actions Marketplace](https://github.com/marketplace?type=actions).

## 5. Step-by-step: Enable GitHub Actions

Let's begin with a simple setup using the GitHub website. No PowerShell, Git command line or local installation is required.

We will use an existing repository in your GitHub organization.

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/0ea79d93-496a-4925-afaa-2350f9954a08" />


### Step 1 – Open your repository

1. Sign in to GitHub.
2. Open your GitHub organization.
3. Select a repository you control.
4. Click the **Actions** tab.

For the first experiment, I recommend using a separate repository named:

```text
ESP32-GitHub-Actions-Lab
```

This prevents experimental workflow files from affecting your actual teaching materials.

### Step 2 – Enable Actions

If Actions is not enabled:

1. Open repository **Settings**.
2. Select **Actions**.
3. Select **General**.
4. Check the **Actions permissions**.
5. Allow the workflows you intend to use.

For a beginner experiment, the default GitHub-provided actions are sufficient.

If this is an organization repository, the organization-level Actions policy may also restrict what individual repositories can run.

### Step 3 – Create your first workflow

You can create a workflow directly through the browser.

1. Open the repository.
2. Click **Add file**.
3. Select **Create new file**.
4. Enter this filename:

```text
.github/workflows/hello-world.yml
```

GitHub automatically creates the necessary folders when you enter the full path.

### 5. Paste the following YAML:

```yaml

name: My First GitHub Action

on:
  push:
    branches:
      - main

  workflow_dispatch:

jobs:
  hello-world:
    name: Hello World Job
    runs-on: ubuntu-latest

    steps:
      - name: Display greeting
        run: echo "Hello from GitHub Actions!"

      - name: Display repository
        run: 'echo "Repository: $GITHUB_REPOSITORY"'

      - name: Display current date
        run: date

```

Click **Commit changes**.

Commit directly to the `main` branch.

Congratulations! You have created your first workflow.

### Step 4 – Execute the workflow

There are two ways to execute it.

**Method A – Automatic execution**

Since the workflow contains:

```yaml
on:
  push:
    branches:
      - main
```

Every new push to the `main` branch triggers the workflow.

**Method B – Manual execution**

1. Open the repository's **Actions** tab.
2. Select **My First GitHub Action**.
3. Click **Run workflow**.
4. Select the `main` branch.
5. Click **Run workflow** again.



<img width="80%" height="auto" alt="image" src="https://github.com/user-attachments/assets/ec6534c4-2cf6-4d38-afda-bd6354736918" />


### Step 5 – Examine the execution results

1. Open the **Actions** tab.
2. Select the latest workflow run.
3. Click **Hello World Job**.
4. Expand each execution step.
5. Examine the output.

You should see something similar to:

```text
Hello from GitHub Actions!

Repository: your-org/ESP32-GitHub-Actions-Lab

Current date: ...
```

**Success**  Workflow completed successfully


You have now demonstrated the complete workflow lifecycle:

```text
Trigger → Workflow → Job → Runner → Steps → Results
```

<img width="80%" height="auto" alt="image" src="https://github.com/user-attachments/assets/2dd809fa-eed3-42d5-8585-fac9fb52fc11" />



## 6. Practical project: Automatically compile ESP32-S3 firmware

Now let's move from a simple demonstration to something useful for your embedded systems teaching environment.

### Recommended practical exercise

We will create a workflow that automatically compiles an Arduino ESP32-S3 sketch.

The setup will use:

- Arduino CLI
- ESP32 Arduino Core 3.3.12
- ESP32-S3 target
- Ubuntu GitHub-hosted runner

This is compatible with your existing Arduino IDE 2.3.10 and ESP32 Arduino Core 3.3.12 development environment.

### Step 1 – Prepare the repository

Create this structure in your GitHub repository:

```text
ESP32-GitHub-Actions-Lab/
│
├── ESP32S3_Blink/
│   └── ESP32S3_Blink.ino
│
└── .github/
    └── workflows/
        └── esp32-build.yml
```

The folder and `.ino` filename should have matching names.

### Step 2 – Create the Arduino sketch

Create `ESP32S3_Blink/ESP32S3_Blink.ino`:

```cpp
#define LED_PIN 2

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  Serial.println("ESP32-S3 started");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);

  digitalWrite(LED_PIN, LOW);
  delay(500);
}
```

This is a basic compilation exercise. GPIO 2 is only an example; the actual LED connection depends on your ESP32-S3 development board.

### Step 3 – Create the ESP32 compilation workflow

Create the file:

```text
.github/workflows/esp32-build.yml
```

Paste this configuration:

```yaml
name: ESP32-S3 Firmware Build

on:
  push:
    branches:
      - main

  pull_request:
    branches:
      - main

  workflow_dispatch:

permissions:
  contents: read

jobs:
  build:
    name: Compile ESP32-S3
    runs-on: ubuntu-latest

    steps:
      # 1. Download repository source code
      - name: Checkout repository
        uses: actions/checkout@v4

      # 2. Install Arduino CLI
      - name: Install Arduino CLI
        uses: arduino/setup-arduino-cli@v2

      # 3. Configure ESP32 board package
      - name: Configure ESP32 board manager
        run: |
          arduino-cli config init
          arduino-cli config add board_manager.additional_urls \
            https://espressif.github.io/arduino-esp32/package_esp32_index.json

      # 4. Download board definitions
      - name: Update board index
        run: arduino-cli core update-index

      # 5. Install ESP32 Arduino Core
      - name: Install ESP32 Core
        run: |
          arduino-cli core install esp32:esp32@3.3.12

      # 6. Compile firmware
      - name: Compile ESP32-S3 firmware
        run: |
          arduino-cli compile \
            --fqbn esp32:esp32:esp32s3 \
            --output-dir ./build \
            ./ESP32S3_Blink

      # 7. Display generated firmware
      - name: List firmware files
        run: ls -lh ./build
```

This example uses the official Arduino CLI setup action and Espressif's Arduino ESP32 board package.

References:

- [Arduino CLI setup action](https://github.com/arduino/setup-arduino-cli)
- [Arduino ESP32 documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [Arduino CLI documentation](https://arduino.github.io/arduino-cli/).

### Step 4 – Commit and execute

1. Click **Commit changes**.
2. Commit both the Arduino sketch and workflow YAML file.
3. Open the **Actions** tab.
4. Select **ESP32-S3 Firmware Build**.
5. Click the latest workflow execution.
6. Select the **Compile ESP32-S3** job.

You should see these operations:

```text
Expected workflow execution

1. Checkout repository
Expected: PASS

2. Install Arduino CLI
Expected: PASS

3. Configure ESP32 board manager
Expected: PASS

4. Install ESP32 Core 3.3.12
Expected: PASS

5. Compile ESP32-S3 firmware
Expected: PASS

6. List firmware files
Expected: PASS

These are the expected results, not a claim that the workflow has already been executed.

<img width="80%" height="auto" alt="image" src="https://github.com/user-attachments/assets/b7603e29-085c-4336-8a01-56eb3dffca3d" />


```

### Step 5 – Save the compiled firmware as an artifact

Currently, the firmware is generated inside the runner's temporary environment.

When the job finishes, the runner is discarded. If you want to retain the firmware, add another step after **List firmware files**:

```yaml
      - name: Upload firmware artifact
        uses: actions/upload-artifact@v4
        with:
          name: ESP32-S3-Firmware
          path: build/*.bin
          retention-days: 7
```

Now GitHub saves the generated firmware as a downloadable artifact.

To download it:

1. Open the successful workflow run.
2. Scroll to the bottom of the summary page.
3. Find **Artifacts**.
4. Click **ESP32-S3-Firmware**.

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/3370d2ea-83ae-466c-a069-12005401d10a" />


This is useful for your firmware distribution and testing exercises.

**Important for your ESP32-S3 N16R8 board:** The example above compiles for the generic ESP32-S3 target. It does not yet guarantee that the generated firmware uses your exact 16 MB Flash, OPI PSRAM and other board-menu settings. Those must be configured through the appropriate Arduino CLI board options. Also, compilation does not physically upload or test the firmware on your development board.

## 7. Integrating GitHub Actions with GitHub Classroom

This is where GitHub Actions becomes particularly valuable for your teaching environment.

Imagine you have 30 students working on an ESP32 GPIO assignment.

Without automation:

1. Students submit their source code.
2. You download each repository.
3. Open Arduino IDE.
4. Compile each assignment.
5. Record compilation errors.
6. Return feedback to students.

With GitHub Actions:

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/7b19f3c7-8a80-44e5-a2b9-5febe44c736b" />


The process becomes:

1. Instructor creates an assignment using GitHub Classroom.
2. Students accept the assignment.
3. GitHub creates individual student repositories.
4. Students upload their Arduino code.
5. GitHub Actions automatically starts.
6. The workflow compiles the code.
7. Compilation results are recorded.
8. Instructor reviews the results.

### Suggested automated assessment structure

For example, an ESP32 GPIO assignment could have the following checks:

| Assessment | Automated check |
|---|---|
| Source code submitted | Check required `.ino` file |
| Correct board target | Compile for ESP32-S3 |
| GPIO initialization | Check required declarations |
| Digital output | Check compilation and required function |
| Documentation | Check `README.md` |
| Compilation | Build must succeed |

Note that source-code checks can verify the presence of required functions, but they cannot prove that the student's physical circuit works correctly.

### Where should the workflow be stored?

I recommend placing the workflow in your GitHub Classroom assignment template repository:

```text
ESP32_GPIO_Assignment/
│
├── ESP32_GPIO_Assignment.ino
├── README.md
│
└── .github/
    └── workflows/
        └── auto-build.yml
```

When GitHub Classroom creates student repositories from the template, the workflow configuration can be included.

However, verify that Actions is enabled and permitted in the generated repositories. Organization policies, workflow approval requirements and permissions can affect execution.

For private student repositories, make sure the organization has sufficient Actions minutes.

## 8. GitHub Actions usage limitations on Free accounts

Since you are using GitHub Free, this is worth understanding before deploying workflows across an entire class.

**GitHub Free**

| Resource | Free allowance |
|---|---|
| Actions minutes | 2,000/month |
| Artifact storage | 500 MB |
| Cache storage | 10 GB per repository |
| Public repository standard runners | Free |
| Self-hosted runners | No GitHub runner-minute charge |

The 500 MB artifact allowance is shared with GitHub Packages storage. The cache allowance is separate.

For private repositories, standard hosted-runner usage consumes the monthly minutes. Public repositories using standard hosted runners do not consume that allowance.  
GitHub Docs+1

### Example: Estimate for your classroom

Assume:

- 30 students
- 10 assignments per student
- Each compilation takes 2 minutes

Total: *30 * 10 * 2 = 600 minutes*

Monthly Actions usage estimate:

<img width="60%" height="auto" alt="image" src="https://github.com/user-attachments/assets/f8509963-b0a7-40fe-87ee-e9c773b8f6a5" />


```text
600 / 2,000
30% of monthly allowance
1,400 minutes remaining
```

This is an illustrative estimate assuming private repositories under a Free billing account. Actual usage depends on runner type, operating system, execution time and billing-account ownership.

For your classroom, I recommend compiling only when students push code or submit a pull request, rather than on every minor repository event.

## 9. Monitoring and troubleshooting

GitHub provides a dashboard for monitoring workflow executions.

Open the repository's **Actions** tab.

You can examine:

| Information | What you learn |
|---|---|
| Workflow status | Successful, failed or cancelled |
| Execution duration | How long compilation takes |
| Job logs | Which operation failed |
| Trigger event | What started the workflow |
| Commit information | Which source-code revision was tested |
| Artifacts | What files were generated |

### Common errors in ESP32 workflows

| Error | Possible reason |
|---|---|
| Sketch not found | Incorrect folder or filename |
| Board not found | ESP32 core not installed |
| Library not found | Required library missing from workflow |
| Compilation failed | Source-code or dependency error |
| Permission denied | Repository Actions permissions |
| Artifact not found | Incorrect output directory or file pattern |

**One important point:** GitHub-hosted runners are temporary cloud computers. They do not retain installed ESP32 libraries or build files between separate workflow runs unless you configure caching or artifact storage.

## 10. Security considerations

For your ESP32 secure OTA and GitLab integration work, I recommend these practices:

- Never hardcode GitLab deployment tokens in YAML files.
- Store confidential credentials using GitHub Actions Secrets.
- Give workflows only the permissions they require.
- Pin third-party actions to reviewed versions or immutable commit SHAs for production workflows.
- Avoid executing untrusted student code in workflows that have access to deployment secrets.
- Require instructor approval before publishing firmware or deploying production websites.

GitHub Actions can become part of your secure software development lifecycle, but the workflow itself must also be secured.

## 11. Recommended learning roadmap

I suggest introducing GitHub Actions to your students and yourself in four stages.

```text
GitHub Actions learning roadmap

Stage 1 – Fundamentals
Start here
- Create a workflow.
- Understand YAML syntax.
- Configure event triggers.
- Execute manual workflows.
- Read execution logs.

Stage 2 – ESP32 Continuous Integration
- Install Arduino CLI.
- Install ESP32 Arduino Core.
- Compile ESP32-S3 projects.
- Manage external libraries.
- Save firmware artifacts.

Stage 3 – GitHub Classroom Automation
- Integrate workflows into assignment templates.
- Automatically compile student submissions.
- Generate assessment reports.
- Validate required source files.
- Monitor student submission results.

Stage 4 – Advanced CI/CD and Security
- Matrix builds.
- GitHub Secrets.
- Reusable workflows.
- Automated firmware releases.
- GitLab Package Registry integration.
- Automated website deployment.
```

## 12. Official learning resources

These are useful references:

- [GitHub Actions – Official Documentation](https://docs.github.com/en/actions)
- [Writing workflows](https://docs.github.com/en/actions/writing-workflows)
- [Workflow events and triggers](https://docs.github.com/en/actions/using-workflows/events-that-trigger-workflows)
- [GitHub Actions security](https://docs.github.com/en/actions/security-for-github-actions)
- [GitHub Actions Marketplace](https://github.com/marketplace?type=actions)
- [GitHub Actions features](https://github.com/features/actions)

## My recommendation for your next implementation

I would take your existing GitHub organization and GitHub Classroom setup and build a reusable ESP32-S3 CI/CD workflow template.

The eventual architecture would look like this:

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/6afc75f3-406a-41eb-9c2b-074e5f77f425" />



```text
GitHub Classroom
Student assignment repositories

GitHub Actions
Automated build and assessment

Build successful
Generate firmware BIN

Build failed
Return compilation logs

Instructor assessment dashboard
Build status, reports and downloadable firmware
```

The most valuable next step is to develop a reusable workflow that supports your exact ESP32-S3 N16R8 configuration, Arduino Core 3.3.12, required libraries and compilation settings. Once tested in one repository, you can introduce it into your GitHub Classroom assignment templates and scale it across your students.
