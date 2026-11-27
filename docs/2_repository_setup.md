# 2 · Team repository setup

**One** team member does steps 1–4. Everyone else then clones.
Deadline: **before Review 1 (27 Nov, 10:00 PM IST)**. Earlier is better: do it before kickoff if you can.

## 1. Create your private repository from the template

1. Open **[Use this template → Create a new repository](https://github.com/SiliconSprint2026/SiliconSprint_DigitalTrack/generate)**
   (or the green **Use this template** button on the
   [track page](https://github.com/SiliconSprint2026/SiliconSprint_DigitalTrack)).
2. Owner: your own GitHub account.
3. Repository name: **`ss26-digital-<team-name>`** (lowercase, no spaces).
4. Visibility: **Private** (required).
5. Leave "Include all branches" **unticked**. Click **Create repository**.

> **Do not fork.** A fork of a public repository is always public: anyone could
> see your solution, and it cannot be made private later. Forked repositories are
> not accepted for submission. If you forked by mistake, delete the fork and use
> the template.

## 2. Add collaborators

Your repository → **Settings → Collaborators → Add people**:

* every team member (role **Write**)
* the organizer account **`SiliconSprint2026`** (role **Read** is enough). The judges review your work through this account

Each invitation must be **accepted** (by email or on github.com/notifications).
Invitations expire after 7 days.

## 3. Fill in your team file

Edit `submission/TEAM.md` (team name, members, GitHub usernames), commit and push.
You can do this in the browser: open the file → pencil icon → **Commit changes**.

## 4. Register

Submit your track, team name and repository URL. The registration link is shared on the [SiliconSprint 2026 WhatsApp channel](https://chat.whatsapp.com/DpBT2pWoCFdBt4d3ggV3fX).

Before Gate 1 a volunteer checks two things:
* your repository URL shows **404** to a logged-out visitor (it is private)
* the `SiliconSprint2026` account can open it

## 5. Everyone: clone

```bash
git config --global core.autocrlf false        # Windows: required (see 1_docker_setup.md §5)
git clone https://github.com/<owner>/ss26-digital-<team-name>.git
cd ss26-digital-<team-name>
```

Then do the setup check in [1_docker_setup.md §4](1_docker_setup.md#4-setup-check).

## Repository rules

* Created with **Use this template**, **private** until the awards are announced.
* Commit and **push** often, at least at every mentor checkpoint. Unpushed work does not exist for the judges.
* Do not commit build outputs. `.gitignore` already excludes them; do not edit it.
* Do not edit the files the grader depends on: `tests/common/`, `tools/` (except
  constraints in `rand_gen.py`), `docker/`, the cache RTL.
* No force-push and no history rewriting after code freeze.
* Tip: work on short-lived branches and merge into `main`. Gates and final grading
  use the commit you submit, normally the tip of `main`.

### Getting fixes from the organizers

If the organizers announce a fix to the kit during the event, it is published in the
template repository. Get it with:

```bash
git remote add kit https://github.com/SiliconSprint2026/SiliconSprint_DigitalTrack.git   # once
git fetch kit
git checkout kit/main -- <file named in the announcement>
git commit -m "Apply organizer fix"
```
