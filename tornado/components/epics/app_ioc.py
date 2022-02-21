from __future__ import annotations
from typing import Callable, Dict, List

from pathlib import Path

from ferrite.components.base import Task, Context
from ferrite.components.app import AppBase
from ferrite.components.epics.epics_base import AbstractEpicsBase
from ferrite.components.epics.ioc import IocCross, IocHost
from ferrite.components.epics.app_ioc import AbstractAppIoc


class AppIocHost(AbstractAppIoc, IocHost):

    class BuildTask(AbstractAppIoc.BuildTask, IocHost.BuildTask):
        pass

    class RunTask(Task):

        def __init__(self, owner: AppIocHost, run_fn: Callable[[Path, Path, str], None]) -> None:
            super().__init__()
            self.owner = owner
            self.run_fn = run_fn

        def run(self, ctx: Context) -> None:
            self.run_fn(
                self.owner.epics_base.install_path,
                self.owner.install_path,
                self.owner.arch,
            )

        def dependencies(self) -> List[Task]:
            return [
                self.owner.epics_base.build_task,
                self.owner.build_task,
            ]

    def __init__(
        self,
        ioc_dir: Path,
        ferrite_source_dir: Path,
        target_dir: Path,
        epics_base: AbstractEpicsBase,
        app: AppBase,
    ):
        self.app = app

        super().__init__(
            ioc_dir,
            ferrite_source_dir,
            target_dir,
            epics_base,
            app,
        )

        from tornado.ioc.fakedev import dummy, test
        self.run_task = self.RunTask(self, dummy.run)
        self.test_task = self.RunTask(self, test.run)

    def tasks(self) -> Dict[str, Task]:
        tasks = super().tasks()
        tasks.update({
            "run": self.run_task,
            "test": self.test_task,
        })
        return tasks


class AppIocCross(AbstractAppIoc, IocCross):

    class BuildTask(AbstractAppIoc.BuildTask, IocCross.BuildTask):
        pass
