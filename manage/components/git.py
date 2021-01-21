from __future__ import annotations
import os
import logging
from manage.components.base import Component, Task, TaskArgs
from manage.utils.run import run
from manage.paths import target_dir

class GitCloneTask(Task):
    def __init__(self, remote: str, path: str, branch: str=None):
        super().__init__()
        self.remote = remote
        self.path = path
        self.branch = branch

    def run(self, args: TaskArgs):
        if os.path.exists(self.path):
            logging.info(f"Repo '{self.remote}' is cloned already")
            return
        run(
            ["git", "clone", self.remote, os.path.basename(self.path)],
            cwd=os.path.dirname(self.path)
        )
        if self.branch:
            run(["git", "checkout", self.branch], cwd=self.path)
        run(["git", "submodule", "update", "--init", "--recursive"], cwd=self.path)

class Repo(Component):
    def __init__(self, remote: str, name: str, branch: str=None):
        super().__init__()

        self.remote = remote
        self.name = name
        self.path = os.path.join(target_dir, name)
        self.branch = branch

        self.clone_task = GitCloneTask(self.remote, self.path, self.branch)

    def tasks(self) -> dict[str, Task]:
        return {
            "clone": self.clone_task,
        }
