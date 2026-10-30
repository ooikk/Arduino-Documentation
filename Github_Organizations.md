
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

GitHub's repository roles include Read, Triage, Write, Maintain, and Admin, with different capabilities. ([GitHub repository roles](https://docs.github.com/en/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles?utm_source=chatgpt.com))

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

```
git clone https://github.com/esp32-course-2026/01-lecture-notes.git
```

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

