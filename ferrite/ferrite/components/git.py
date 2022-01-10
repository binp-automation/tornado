from __future__ import annotations
from typing import Dict, List, Optional

import os
import shutil
import logging

from dataclasses import dataclass
from ferrite.utils.run import run, RunError
from ferrite.components.base import Component, Task, Context
from ferrite.manage.paths import TARGET_DIR


def clone(path: str, remote: str, branch: Optional[str] = None, clean: bool = False, quiet: bool = False) -> bool:
    # FIXME: Pull if update available
    if os.path.exists(path):
        logging.info(f"Repo '{remote}' is cloned already")
        return False
    try:
        os.makedirs(TARGET_DIR, exist_ok=True)
        run(
            ["git", "clone", remote, os.path.basename(path)],
            cwd=os.path.dirname(path),
            add_env={"GIT_TERMINAL_PROMPT": "0"},
            quiet=quiet,
        )
        if branch:
            run(["git", "checkout", branch], cwd=path, quiet=quiet)
        run(["git", "submodule", "update", "--init", "--recursive"], cwd=path, quiet=quiet)
    except RunError:
        if os.path.exists(path):
            shutil.rmtree(path)
        raise
    if clean:
        shutil.rmtree(os.path.join(path, ".git"))
    return True


@dataclass
class RepoSource:
    remote: str
    branch: Optional[str]


class GitCloneTask(Task):

    def __init__(self, path: str, sources: List[RepoSource]):
        super().__init__()
        self.path = path
        self.sources = sources

    def run(self, ctx: Context) -> None:
        last_error = None
        for source in self.sources:
            try:
                clone(self.path, source.remote, source.branch, clean=True, quiet=ctx.capture)
            except RunError as e:
                last_error = e
                continue
        if last_error is not None:
            raise last_error

    def artifacts(self) -> List[str]:
        return [self.path]


class RepoList(Component):

    def __init__(self, name: str, sources: List[RepoSource]):
        super().__init__()

        self.name = name
        self.path = os.path.join(TARGET_DIR, name)
        self.sources = sources

        self.clone_task = GitCloneTask(self.path, self.sources)

    def tasks(self) -> Dict[str, Task]:
        return {
            "clone": self.clone_task,
        }


class Repo(RepoList):

    def __init__(self, name: str, remote: str, branch: Optional[str] = None):
        super().__init__(name, [RepoSource(remote, branch)])
